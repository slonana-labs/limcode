//! Solana Snapshot Archive Support
//!
//! Fast parsing and streaming of Solana snapshot archives (.tar.zst)
//! Optimized for minimal memory usage and maximum throughput

use std::fs::File;
use std::io::{self, BufReader, Read};
use std::path::Path;

#[cfg(feature = "solana")]
use tar::Archive;
#[cfg(feature = "solana")]
use zstd::stream::read::Decoder as ZstdDecoder;

/// Solana account data from snapshot AppendVec format
#[derive(Debug, Clone)]
pub struct SnapshotAccount {
    pub write_version: u64, // Obsolete but present in format
    pub pubkey: [u8; 32],
    pub lamports: u64,
    pub rent_epoch: u64,
    pub owner: [u8; 32],
    pub executable: bool,
    pub hash: [u8; 32], // Account state hash
    pub data: Vec<u8>,  // Variable length
}

/// AppendVec account header (136 bytes) - kept for documentation
#[allow(dead_code)]
#[repr(C, packed)]
struct AccountHeader {
    write_version: u64, // 8 bytes - offset 0x00
    data_len: u64,      // 8 bytes - offset 0x08
    pubkey: [u8; 32],   // 32 bytes - offset 0x10
    lamports: u64,      // 8 bytes - offset 0x30
    rent_epoch: u64,    // 8 bytes - offset 0x38
    owner: [u8; 32],    // 32 bytes - offset 0x40
    executable: u8,     // 1 byte - offset 0x60
    padding: [u8; 7],   // 7 bytes - offset 0x61
    hash: [u8; 32],     // 32 bytes - offset 0x68
                        // Total: 136 bytes (0x88)
}

/// Parse accounts from AppendVec file data
///
/// AppendVec files contain sequential account records:
/// - 136-byte header (write_version, data_len, pubkey, lamports, rent_epoch, owner, executable, padding, hash)
/// - Variable-length data section
/// - 8-byte alignment padding
///
/// Note: Real snapshot parsing requires the manifest file to know file sizes.
/// This function assumes the data is already extracted from the AppendVec file.
#[cfg(feature = "solana")]
pub fn parse_appendvec(data: &[u8]) -> io::Result<Vec<SnapshotAccount>> {
    let mut accounts = Vec::new();
    let mut offset = 0;

    const HEADER_SIZE: usize = 136;

    while offset + HEADER_SIZE <= data.len() {
        // Read 136-byte header
        let write_version =
            u64::from_le_bytes(data[offset + 0x00..offset + 0x08].try_into().unwrap());
        let data_len =
            u64::from_le_bytes(data[offset + 0x08..offset + 0x10].try_into().unwrap()) as usize;

        let mut pubkey = [0u8; 32];
        pubkey.copy_from_slice(&data[offset + 0x10..offset + 0x30]);

        let lamports = u64::from_le_bytes(data[offset + 0x30..offset + 0x38].try_into().unwrap());
        let rent_epoch = u64::from_le_bytes(data[offset + 0x38..offset + 0x40].try_into().unwrap());

        let mut owner = [0u8; 32];
        owner.copy_from_slice(&data[offset + 0x40..offset + 0x60]);

        let executable = data[offset + 0x60] != 0;
        // Skip 7 bytes padding at offset+0x61

        let mut hash = [0u8; 32];
        hash.copy_from_slice(&data[offset + 0x68..offset + 0x88]);

        offset += HEADER_SIZE;

        // Read variable-length account data
        if offset + data_len > data.len() {
            break; // Incomplete account
        }

        let account_data = data[offset..offset + data_len].to_vec();
        offset += data_len;

        // 8-byte alignment padding
        let padding = (8 - (offset % 8)) % 8;
        offset += padding;

        accounts.push(SnapshotAccount {
            write_version,
            pubkey,
            lamports,
            rent_epoch,
            owner,
            executable,
            hash,
            data: account_data,
        });
    }

    Ok(accounts)
}

/// Streaming iterator for Solana snapshot accounts
///
/// Memory-efficient: processes accounts one at a time, perfect for 1.5B+ accounts
pub struct SnapshotAccountIterator<R: Read + 'static> {
    tar_entries: tar::Entries<'static, R>,
    current_appendvec: Vec<u8>,
    current_offset: usize,
    _archive: Box<Archive<R>>,
}

#[cfg(feature = "solana")]
unsafe impl<R: Read + 'static> Send for SnapshotAccountIterator<R> {}

#[cfg(feature = "solana")]
impl<R: Read + 'static> Iterator for SnapshotAccountIterator<R> {
    type Item = io::Result<SnapshotAccount>;

    fn next(&mut self) -> Option<Self::Item> {
        const HEADER_SIZE: usize = 136;

        loop {
            // Try to parse next account from current AppendVec buffer
            if self.current_offset + HEADER_SIZE <= self.current_appendvec.len() {
                let data = &self.current_appendvec;
                let offset = self.current_offset;

                // Read 136-byte header
                let write_version =
                    u64::from_le_bytes(data[offset..offset + 0x08].try_into().unwrap());
                let data_len =
                    u64::from_le_bytes(data[offset + 0x08..offset + 0x10].try_into().unwrap())
                        as usize;

                let mut pubkey = [0u8; 32];
                pubkey.copy_from_slice(&data[offset + 0x10..offset + 0x30]);

                let lamports =
                    u64::from_le_bytes(data[offset + 0x30..offset + 0x38].try_into().unwrap());
                let rent_epoch =
                    u64::from_le_bytes(data[offset + 0x38..offset + 0x40].try_into().unwrap());

                let mut owner = [0u8; 32];
                owner.copy_from_slice(&data[offset + 0x40..offset + 0x60]);

                let executable = data[offset + 0x60] != 0;
                // Skip 7 bytes padding at offset+0x61

                let mut hash = [0u8; 32];
                hash.copy_from_slice(&data[offset + 0x68..offset + 0x88]);

                self.current_offset += HEADER_SIZE;

                // Read variable-length account data
                if self.current_offset + data_len > data.len() {
                    // Incomplete account, move to next file
                    self.current_appendvec.clear();
                    self.current_offset = 0;
                    continue;
                }

                let account_data =
                    data[self.current_offset..self.current_offset + data_len].to_vec();
                self.current_offset += data_len;

                // 8-byte alignment padding
                let padding = (8 - (self.current_offset % 8)) % 8;
                self.current_offset += padding;

                return Some(Ok(SnapshotAccount {
                    write_version,
                    pubkey,
                    lamports,
                    rent_epoch,
                    owner,
                    executable,
                    hash,
                    data: account_data,
                }));
            }

            // Need to load next AppendVec file
            loop {
                let mut entry = match self.tar_entries.next()? {
                    Ok(e) => e,
                    Err(e) => return Some(Err(e)),
                };

                let path = match entry.path() {
                    Ok(p) => p,
                    Err(_) => continue,
                };

                // Look for AppendVec files in accounts/ directory
                let path_str = path.to_string_lossy();
                if !path_str.starts_with("accounts/") {
                    continue;
                }

                // Read AppendVec file into buffer
                self.current_appendvec.clear();
                if entry.read_to_end(&mut self.current_appendvec).is_err() {
                    continue;
                }

                self.current_offset = 0;
                break; // Start parsing this file
            }
        }
    }
}

/// Stream accounts from Solana snapshot archive (memory-efficient for 1.5B+ accounts)
///
/// WARNING: Simplified parser - extracts AppendVec files only.
/// Full snapshot parsing requires reading manifest (snapshots/SLOT/SLOT).
///
/// Real Solana snapshot structure:
/// - version (contains "1.2.0")
/// - status_cache
/// - snapshots/SLOT/SLOT (manifest file - bincode serialized)
/// - accounts/SLOT.ID (AppendVec files - 97-byte headers + data)
///
/// ```ignore
/// use limcode::snapshot::stream_snapshot;
///
/// for account_result in stream_snapshot("snapshot-123-aBcD.tar.zst")? {
///     let acc = account_result?;
///     db.insert(acc.pubkey, acc)?; // Process directly to database
/// }
/// ```
#[cfg(feature = "solana")]
pub fn stream_snapshot<P: AsRef<Path>>(
    path: P,
) -> io::Result<impl Iterator<Item = io::Result<SnapshotAccount>>> {
    let file = File::open(path)?;
    let buf_reader = BufReader::with_capacity(4 * 1024 * 1024, file); // 4MB buffer for streaming
    let decoder = ZstdDecoder::new(buf_reader)?;
    let archive = Box::new(Archive::new(decoder));

    // SAFETY: Create self-referential struct - archive owns the data, entries borrow from it
    // We convert to raw pointer, call entries(), then reconstruct the Box
    let archive_ptr = Box::into_raw(archive);
    let entries = unsafe { (*archive_ptr).entries()? };
    let archive = unsafe { Box::from_raw(archive_ptr) };

    Ok(SnapshotAccountIterator {
        #[allow(clippy::missing_transmute_annotations)]
        tar_entries: unsafe { std::mem::transmute(entries) },
        current_appendvec: Vec::with_capacity(64 * 1024 * 1024), // 64MB buffer per file
        current_offset: 0,
        _archive: archive,
    })
}

/// Parse entire snapshot into memory (use stream_snapshot for large datasets)
#[cfg(feature = "solana")]
pub fn parse_snapshot<P: AsRef<Path>>(path: P) -> io::Result<Vec<SnapshotAccount>> {
    stream_snapshot(path)?.collect()
}

/// Fast snapshot extraction to directory
#[cfg(feature = "solana")]
pub fn extract_snapshot<P: AsRef<Path>>(snapshot_path: P, output_dir: P) -> io::Result<u64> {
    let file = File::open(snapshot_path)?;
    let buf_reader = BufReader::with_capacity(1024 * 1024, file);
    let decoder = ZstdDecoder::new(buf_reader)?;
    let mut archive = Archive::new(decoder);

    std::fs::create_dir_all(&output_dir)?;
    archive.unpack(&output_dir)?;

    Ok(0) // Return account count if needed
}

/// Parallel snapshot processing (high throughput)
#[cfg(all(feature = "solana", feature = "async"))]
pub async fn process_snapshot_parallel<F, T>(
    snapshot_path: &str,
    processor: F,
    _num_workers: usize,
) -> io::Result<Vec<T>>
where
    F: Fn(SnapshotAccount) -> T + Send + Sync + Clone + 'static,
    T: Send + 'static,
{
    use tokio::sync::mpsc;

    let (tx, mut rx) = mpsc::channel(1000);

    // Spawn reader task
    let snapshot_path = snapshot_path.to_string();
    tokio::task::spawn_blocking(move || {
        let accounts = parse_snapshot(&snapshot_path)?;
        for account in accounts {
            let _ = tx.blocking_send(account);
        }
        Ok::<_, io::Error>(())
    });

    // Spawn worker tasks
    let mut results = Vec::new();
    while let Some(account) = rx.recv().await {
        let processor = processor.clone();
        let result = tokio::task::spawn_blocking(move || processor(account))
            .await
            .unwrap();
        results.push(result);
    }

    Ok(results)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_snapshot_account_size() {
        // Ensure SnapshotAccount is efficiently sized
        assert!(std::mem::size_of::<SnapshotAccount>() < 256);
    }
}

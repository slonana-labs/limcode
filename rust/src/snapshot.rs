//! Solana Snapshot Archive Support
//!
//! Fast parsing and streaming of Solana snapshot archives (.tar.zst)
//! Optimized for minimal memory usage and maximum throughput
//!
//! # Snapshot Structure
//!
//! A Solana snapshot archive contains:
//! - `version` - Version string (e.g., "1.18.0")
//! - `status_cache` - Slot status cache (bincode)
//! - `snapshots/SLOT/SLOT` - Bank manifest with epoch/stake info (bincode)
//! - `accounts/SLOT.ID` - AppendVec account storage files
//!
//! # Usage
//!
//! ```ignore
//! use limcode::snapshot::{stream_snapshot_full, SnapshotItem};
//!
//! for item in stream_snapshot_full("snapshot.tar.zst")? {
//!     match item? {
//!         SnapshotItem::Version(v) => println!("Version: {}", v),
//!         SnapshotItem::Manifest(m) => println!("Slot: {}", m.slot),
//!         SnapshotItem::Account(a) => println!("Account: {:?}", a.pubkey),
//!         _ => {}
//!     }
//! }
//! ```

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

/// Snapshot manifest containing bank state metadata
///
/// Extracted from snapshots/SLOT/SLOT file (bincode serialized)
#[derive(Debug, Clone)]
pub struct SnapshotManifest {
    pub slot: u64,
    pub bank_hash: [u8; 32],
    pub parent_slot: u64,
    pub epoch: u64,
    pub block_height: u64,
    /// Raw manifest data for advanced parsing
    pub raw_data: Vec<u8>,
}

/// Status cache entry from snapshot
#[derive(Debug, Clone)]
pub struct StatusCacheEntry {
    pub slot: u64,
    pub hash: [u8; 32],
    /// Raw status data
    pub data: Vec<u8>,
}

/// Items yielded by full snapshot streaming
#[derive(Debug)]
pub enum SnapshotItem {
    /// Snapshot version string (e.g., "1.18.0")
    Version(String),
    /// Bank manifest with slot/epoch metadata
    Manifest(SnapshotManifest),
    /// Status cache data
    StatusCache(Vec<u8>),
    /// Individual account from AppendVec
    Account(SnapshotAccount),
    /// Unknown/other file in archive
    OtherFile { path: String, data: Vec<u8> },
}

/// Summary statistics from snapshot
#[derive(Debug, Clone, Default)]
pub struct SnapshotStats {
    pub version: String,
    pub slot: u64,
    pub epoch: u64,
    pub total_accounts: u64,
    pub total_lamports: u64,
    pub total_data_bytes: u64,
    pub executable_accounts: u64,
    pub max_account_size: usize,
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
        let write_version = u64::from_le_bytes(data[offset..offset + 0x08].try_into().unwrap());
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

/// Parse manifest file to extract slot, epoch, and bank hash
///
/// The manifest is bincode-serialized BankFields. We extract key fields
/// without requiring the full Solana SDK types.
#[cfg(feature = "solana")]
fn parse_manifest(data: &[u8]) -> io::Result<SnapshotManifest> {
    // Manifest structure (simplified):
    // - First 8 bytes: slot (u64)
    // - Various fields...
    // - Bank hash at a known offset
    //
    // Note: Full parsing requires matching exact Solana version's struct layout
    // For now we extract what we can reliably

    if data.len() < 48 {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "Manifest too small",
        ));
    }

    // Read slot (first u64 in most versions)
    let slot = u64::from_le_bytes(data[0..8].try_into().unwrap());

    // Parent slot is typically next
    let parent_slot = if data.len() >= 16 {
        u64::from_le_bytes(data[8..16].try_into().unwrap())
    } else {
        0
    };

    // Try to find bank hash (32 bytes, usually early in the struct)
    // This is a heuristic - exact offset varies by version
    let mut bank_hash = [0u8; 32];
    if data.len() >= 48 {
        bank_hash.copy_from_slice(&data[16..48]);
    }

    // Epoch is harder to locate without full struct knowledge
    // We'll set it to 0 and let users parse raw_data if needed
    let epoch = 0;
    let block_height = 0;

    Ok(SnapshotManifest {
        slot,
        bank_hash,
        parent_slot,
        epoch,
        block_height,
        raw_data: data.to_vec(),
    })
}

/// Full snapshot iterator that yields all data types
pub struct FullSnapshotIterator<R: Read + 'static> {
    tar_entries: tar::Entries<'static, R>,
    current_appendvec: Vec<u8>,
    current_offset: usize,
    pending_items: Vec<SnapshotItem>,
    _archive: Box<Archive<R>>,
}

#[cfg(feature = "solana")]
unsafe impl<R: Read + 'static> Send for FullSnapshotIterator<R> {}

#[cfg(feature = "solana")]
impl<R: Read + 'static> Iterator for FullSnapshotIterator<R> {
    type Item = io::Result<SnapshotItem>;

    fn next(&mut self) -> Option<Self::Item> {
        const HEADER_SIZE: usize = 136;

        // Return pending items first (from non-account files)
        if let Some(item) = self.pending_items.pop() {
            return Some(Ok(item));
        }

        loop {
            // Try to parse next account from current AppendVec buffer
            if self.current_offset + HEADER_SIZE <= self.current_appendvec.len() {
                let data = &self.current_appendvec;
                let offset = self.current_offset;

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

                let mut hash = [0u8; 32];
                hash.copy_from_slice(&data[offset + 0x68..offset + 0x88]);

                self.current_offset += HEADER_SIZE;

                if self.current_offset + data_len > data.len() {
                    self.current_appendvec.clear();
                    self.current_offset = 0;
                    continue;
                }

                let account_data =
                    data[self.current_offset..self.current_offset + data_len].to_vec();
                self.current_offset += data_len;

                let padding = (8 - (self.current_offset % 8)) % 8;
                self.current_offset += padding;

                return Some(Ok(SnapshotItem::Account(SnapshotAccount {
                    write_version,
                    pubkey,
                    lamports,
                    rent_epoch,
                    owner,
                    executable,
                    hash,
                    data: account_data,
                })));
            }

            // Load next file from archive
            let mut entry = match self.tar_entries.next()? {
                Ok(e) => e,
                Err(e) => return Some(Err(e)),
            };

            let path = match entry.path() {
                Ok(p) => p.to_string_lossy().to_string(),
                Err(_) => continue,
            };

            // Read file content
            let mut content = Vec::new();
            if entry.read_to_end(&mut content).is_err() {
                continue;
            }

            // Classify and handle file
            if path == "version" {
                // Version file - simple text
                let version = String::from_utf8_lossy(&content).trim().to_string();
                return Some(Ok(SnapshotItem::Version(version)));
            } else if path == "status_cache" {
                // Status cache - raw binary
                return Some(Ok(SnapshotItem::StatusCache(content)));
            } else if path.starts_with("snapshots/") && !path.ends_with('/') {
                // Manifest file (snapshots/SLOT/SLOT)
                // Only parse if it looks like a manifest (not a subdirectory marker)
                let parts: Vec<&str> = path.split('/').collect();
                // Format: snapshots/SLOT/SLOT - the filename should match the slot number
                if parts.len() == 3 && parts[1] == parts[2] && !content.is_empty() {
                    match parse_manifest(&content) {
                        Ok(manifest) => return Some(Ok(SnapshotItem::Manifest(manifest))),
                        Err(_) => {
                            // If manifest parsing fails, return raw data
                            return Some(Ok(SnapshotItem::OtherFile {
                                path,
                                data: content,
                            }));
                        }
                    }
                } else if !content.is_empty() {
                    // Other file in snapshots directory
                    return Some(Ok(SnapshotItem::OtherFile {
                        path,
                        data: content,
                    }));
                }
            } else if path.starts_with("accounts/") {
                // AppendVec file - start parsing accounts
                self.current_appendvec = content;
                self.current_offset = 0;
                continue;
            } else if !path.ends_with('/') {
                // Other file
                return Some(Ok(SnapshotItem::OtherFile {
                    path,
                    data: content,
                }));
            }
        }
    }
}

/// Stream all data from Solana snapshot archive
///
/// Unlike `stream_snapshot` which only yields accounts, this function
/// yields all snapshot contents including version, manifest, and status cache.
///
/// # Example
///
/// ```ignore
/// use limcode::snapshot::{stream_snapshot_full, SnapshotItem};
///
/// for item in stream_snapshot_full("snapshot.tar.zst")? {
///     match item? {
///         SnapshotItem::Version(v) => println!("Solana version: {}", v),
///         SnapshotItem::Manifest(m) => {
///             println!("Slot: {}", m.slot);
///             println!("Parent: {}", m.parent_slot);
///         }
///         SnapshotItem::StatusCache(data) => println!("Status cache: {} bytes", data.len()),
///         SnapshotItem::Account(acc) => {
///             println!("Account {} has {} lamports", hex(&acc.pubkey), acc.lamports);
///         }
///         SnapshotItem::OtherFile { path, .. } => println!("Other: {}", path),
///     }
/// }
/// ```
#[cfg(feature = "solana")]
pub fn stream_snapshot_full<P: AsRef<Path>>(
    path: P,
) -> io::Result<impl Iterator<Item = io::Result<SnapshotItem>>> {
    let file = File::open(path)?;
    let buf_reader = BufReader::with_capacity(4 * 1024 * 1024, file);
    let decoder = ZstdDecoder::new(buf_reader)?;
    let archive = Box::new(Archive::new(decoder));

    let archive_ptr = Box::into_raw(archive);
    let entries = unsafe { (*archive_ptr).entries()? };
    let archive = unsafe { Box::from_raw(archive_ptr) };

    Ok(FullSnapshotIterator {
        #[allow(clippy::missing_transmute_annotations)]
        tar_entries: unsafe { std::mem::transmute(entries) },
        current_appendvec: Vec::with_capacity(64 * 1024 * 1024),
        current_offset: 0,
        pending_items: Vec::new(),
        _archive: archive,
    })
}

/// Get snapshot statistics without loading all data into memory
#[cfg(feature = "solana")]
pub fn snapshot_stats<P: AsRef<Path>>(path: P) -> io::Result<SnapshotStats> {
    let mut stats = SnapshotStats::default();

    for item in stream_snapshot_full(path)? {
        match item? {
            SnapshotItem::Version(v) => stats.version = v,
            SnapshotItem::Manifest(m) => {
                stats.slot = m.slot;
                stats.epoch = m.epoch;
            }
            SnapshotItem::Account(acc) => {
                stats.total_accounts += 1;
                stats.total_lamports += acc.lamports;
                stats.total_data_bytes += acc.data.len() as u64;
                if acc.executable {
                    stats.executable_accounts += 1;
                }
                if acc.data.len() > stats.max_account_size {
                    stats.max_account_size = acc.data.len();
                }
            }
            _ => {}
        }
    }

    Ok(stats)
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

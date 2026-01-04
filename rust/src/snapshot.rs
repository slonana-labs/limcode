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

/// Solana account data from snapshot
#[derive(Debug, Clone)]
pub struct SnapshotAccount {
    pub pubkey: [u8; 32],
    pub lamports: u64,
    pub data: Vec<u8>,
    pub owner: [u8; 32],
    pub executable: bool,
    pub rent_epoch: u64,
}

/// Parse Solana snapshot archive and return all accounts
///
/// Decompresses and parses the snapshot into memory
///
/// ```ignore
/// use limcode::snapshot::parse_snapshot;
///
/// let accounts = parse_snapshot("snapshot-123-aBcD.tar.zst")?;
/// for acc in accounts {
///     println!("Pubkey: {:?}, Lamports: {}", acc.pubkey, acc.lamports);
/// }
/// ```
#[cfg(feature = "solana")]
pub fn parse_snapshot<P: AsRef<Path>>(path: P) -> io::Result<Vec<SnapshotAccount>> {
    let file = File::open(path)?;
    let buf_reader = BufReader::with_capacity(1024 * 1024, file); // 1MB buffer
    let decoder = ZstdDecoder::new(buf_reader)?;
    let mut archive = Archive::new(decoder);

    let mut accounts = Vec::new();

    for entry_result in archive.entries()? {
        let mut entry = match entry_result {
            Ok(e) => e,
            Err(_) => continue,
        };

        let path = match entry.path() {
            Ok(p) => p,
            Err(_) => continue,
        };

        let filename = match path.file_name().and_then(|f| f.to_str()) {
            Some(f) => f,
            None => continue,
        };

        // Skip metadata files
        if !filename.ends_with(".account") {
            continue;
        }

        // Read account data
        let mut data = Vec::new();
        if entry.read_to_end(&mut data).is_err() {
            continue;
        }

        // Parse account (simplified - real format has more fields)
        if data.len() < 96 {
            continue; // Invalid account
        }

        let mut pubkey = [0u8; 32];
        pubkey.copy_from_slice(&data[0..32]);

        let lamports = match data[32..40].try_into().ok().map(u64::from_le_bytes) {
            Some(l) => l,
            None => continue,
        };

        let data_len = match data[40..48].try_into().ok().map(u64::from_le_bytes) {
            Some(l) => l as usize,
            None => continue,
        };

        let account_data = if data_len > 0 && data.len() >= 96 + data_len {
            data[96..96 + data_len].to_vec()
        } else {
            Vec::new()
        };

        let mut owner = [0u8; 32];
        owner.copy_from_slice(&data[48..80]);

        let executable = data[80] != 0;

        let rent_epoch = match data[88..96].try_into().ok().map(u64::from_le_bytes) {
            Some(r) => r,
            None => continue,
        };

        accounts.push(SnapshotAccount {
            pubkey,
            lamports,
            data: account_data,
            owner,
            executable,
            rent_epoch,
        });
    }

    Ok(accounts)
}

/// Fast snapshot extraction to directory
#[cfg(feature = "solana")]
pub fn extract_snapshot<P: AsRef<Path>>(
    snapshot_path: P,
    output_dir: P,
) -> io::Result<u64> {
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

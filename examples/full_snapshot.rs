//! Extract all data from Solana snapshot
//!
//! Run with: cargo run --release --features solana --example full_snapshot -- /path/to/snapshot.tar.zst

#[cfg(feature = "solana")]
use limcode::snapshot::{stream_snapshot_full, SnapshotItem};

#[cfg(feature = "solana")]
fn main() -> std::io::Result<()> {
    let snapshot_path = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "/home/larp/snapshots/snapshot-389758228.tar.zst".to_string());

    println!("Full Solana Snapshot Extractor");
    println!("Snapshot: {}\n", snapshot_path);

    let start = std::time::Instant::now();
    let mut version = String::new();
    let mut slot = 0u64;
    let mut parent_slot = 0u64;
    let mut status_cache_size = 0usize;
    let mut account_count = 0u64;
    let mut total_lamports = 0u64;
    let mut total_data_bytes = 0u64;
    let mut executable_count = 0u64;
    let mut other_files: Vec<String> = Vec::new();

    for item in stream_snapshot_full(&snapshot_path)? {
        match item? {
            SnapshotItem::Version(v) => {
                version = v;
                println!("=== METADATA ===");
                println!("Solana Version: {}", version);
            }
            SnapshotItem::Manifest(m) => {
                slot = m.slot;
                parent_slot = m.parent_slot;
                println!("Slot: {}", slot);
                println!("Parent Slot: {}", parent_slot);
                println!("Bank Hash: {}", hex(&m.bank_hash));
                println!("Manifest Size: {} bytes", m.raw_data.len());
                println!();
            }
            SnapshotItem::StatusCache(data) => {
                status_cache_size = data.len();
                println!("Status Cache: {} bytes", status_cache_size);
            }
            SnapshotItem::Account(acc) => {
                account_count += 1;
                total_lamports += acc.lamports;
                total_data_bytes += acc.data.len() as u64;
                if acc.executable {
                    executable_count += 1;
                }

                // Show first 5 accounts
                if account_count <= 5 {
                    println!("Account {}:", account_count);
                    println!("  Pubkey: {}", hex(&acc.pubkey));
                    println!("  Lamports: {} ({:.4} SOL)", acc.lamports, acc.lamports as f64 / 1e9);
                    println!("  Owner: {}", hex(&acc.owner));
                    println!("  Data: {} bytes", acc.data.len());
                    println!();
                }

                // Progress
                if account_count % 10_000_000 == 0 {
                    println!(
                        "Progress: {}M accounts ({:.1}s)",
                        account_count / 1_000_000,
                        start.elapsed().as_secs_f64()
                    );
                }
            }
            SnapshotItem::OtherFile { path, data } => {
                other_files.push(format!("{} ({} bytes)", path, data.len()));
            }
        }
    }

    let elapsed = start.elapsed();

    println!("\n=== SUMMARY ===");
    println!("Version: {}", version);
    println!("Slot: {}", slot);
    println!("Parent Slot: {}", parent_slot);
    println!("Status Cache: {} bytes", status_cache_size);
    println!();
    println!("=== ACCOUNTS ===");
    println!("Total Accounts: {}", account_count);
    println!("Total SOL: {:.2}", total_lamports as f64 / 1e9);
    println!("Total Data: {:.2} GB", total_data_bytes as f64 / 1e9);
    println!("Executable: {}", executable_count);
    println!("Data Accounts: {}", account_count - executable_count);
    println!();
    println!("=== PERFORMANCE ===");
    println!("Time: {:.2}s", elapsed.as_secs_f64());
    println!(
        "Speed: {:.0} accounts/sec",
        account_count as f64 / elapsed.as_secs_f64()
    );

    if !other_files.is_empty() {
        println!("\n=== OTHER FILES ===");
        for f in &other_files {
            println!("  {}", f);
        }
    }

    Ok(())
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{:02x}", b)).collect()
}

#[cfg(not(feature = "solana"))]
fn main() {
    println!("Build with: cargo run --release --features solana --example full_snapshot");
}

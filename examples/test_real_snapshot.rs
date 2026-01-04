//! Test real Solana snapshot parsing

#[cfg(feature = "solana")]
use limcode::snapshot::stream_snapshot;

#[cfg(feature = "solana")]
fn main() -> std::io::Result<()> {
    let snapshot_path = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "/home/larp/larpdevs/osvm-cli/test-ledger/snapshot-1388137-HESbqSY6jVvngkUVSiBMCXY5iYaa1bfo1ApRgCRihJVq.tar.zst".to_string());

    println!("Testing Solana snapshot parser");
    println!("Snapshot: {}\n", snapshot_path);

    let start = std::time::Instant::now();
    let mut count = 0u64;
    let mut total_lamports = 0u64;
    let mut total_data_bytes = 0u64;
    let mut executable_count = 0u64;
    let mut max_data_size = 0usize;

    match stream_snapshot(&snapshot_path) {
        Ok(iterator) => {
            for (i, account_result) in iterator.enumerate() {
                match account_result {
                    Ok(account) => {
                        count += 1;
                        total_lamports += account.lamports;
                        total_data_bytes += account.data.len() as u64;

                        if account.data.len() > max_data_size {
                            max_data_size = account.data.len();
                        }

                        if account.executable {
                            executable_count += 1;
                        }

                        // Show first 10 accounts in detail
                        if i < 10 {
                            println!("Account {}:", i);
                            println!("  Pubkey: {}", hex(&account.pubkey));
                            println!("  Lamports: {}", account.lamports);
                            println!("  Owner: {}", hex(&account.owner));
                            println!("  Executable: {}", account.executable);
                            println!("  Rent epoch: {}", account.rent_epoch);
                            println!("  Hash: {}", hex(&account.hash));
                            println!("  Data size: {} bytes", account.data.len());
                            println!("  Write version: {}", account.write_version);
                            if account.data.len() > 0 && account.data.len() <= 32 {
                                println!("  Data: {}", hex(&account.data));
                            }
                            println!();
                        }

                        // Progress every 10k accounts
                        if count % 10000 == 0 {
                            println!("Processed {} accounts... ({:.2}s)", count, start.elapsed().as_secs_f64());
                        }
                    }
                    Err(e) => {
                        println!("ERROR reading account {}: {}", i, e);
                        break;
                    }
                }
            }

            let elapsed = start.elapsed();
            println!("\n=== RESULTS ===");
            println!("Total accounts: {}", count);
            println!("Total lamports: {} SOL", total_lamports as f64 / 1e9);
            println!("Total data: {:.2} MB", total_data_bytes as f64 / 1_000_000.0);
            println!("Executable accounts: {}", executable_count);
            println!("Data accounts: {}", count - executable_count);
            println!("Max account data size: {} bytes", max_data_size);
            println!("Time: {:.2}s", elapsed.as_secs_f64());
            println!("Speed: {:.0} accounts/sec", count as f64 / elapsed.as_secs_f64());
        }
        Err(e) => {
            println!("ERROR opening snapshot: {}", e);
            return Err(e);
        }
    }

    Ok(())
}

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{:02x}", b)).collect()
}

#[cfg(not(feature = "solana"))]
fn main() {
    println!("Build with: cargo run --release --features solana --example test_real_snapshot");
}

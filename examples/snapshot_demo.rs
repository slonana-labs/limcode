//! Solana Snapshot Parsing Example
//!
//! Demonstrates how to parse Solana snapshot archives (.tar.zst)
//! and extract account data.
//!
//! Build with: cargo build --release --features solana --example snapshot_demo
//! Run with: cargo run --release --features solana --example snapshot_demo

#[cfg(feature = "solana")]
use limcode::snapshot::{extract_snapshot, parse_snapshot, stream_snapshot, SnapshotAccount};

#[cfg(feature = "solana")]
fn main() -> std::io::Result<()> {
    println!("Limcode Solana Snapshot Parser Demo\n");
    println!("⚠️  Real Solana snapshot format:");
    println!("   - AppendVec files: 97-byte headers + variable data");
    println!("   - Files in accounts/ directory (not .account extension)");
    println!("   - Requires manifest for full parsing\n");

    // Example 1: Stream accounts (RECOMMENDED for 1.5B+ accounts)
    println!("Example 1: Stream accounts (memory-efficient)");
    println!("-----------------------------------------------------------");

    let snapshot_path = "snapshot-123-aBcDeF.tar.zst"; // Replace with actual path

    match stream_snapshot(&snapshot_path) {
        Ok(iterator) => {
            let mut count = 0u64;
            let mut total_lamports = 0u64;
            let mut executable_count = 0u64;

            println!("Streaming accounts (one at a time, minimal memory)...");

            for (i, account_result) in iterator.enumerate() {
                match account_result {
                    Ok(account) => {
                        count += 1;
                        total_lamports += account.lamports;
                        if account.executable {
                            executable_count += 1;
                        }

                        // Show first 5 accounts
                        if i < 5 {
                            print_account(i, &account);
                        }

                        // Process to database here: db.insert(account.pubkey, account)?;
                    }
                    Err(e) => {
                        println!("Error reading account {}: {}", i, e);
                    }
                }
            }

            println!("\nStatistics:");
            println!("  Total accounts: {}", count);
            println!("  Total lamports: {}", total_lamports);
            println!("  Executable accounts: {}", executable_count);
            println!("  Data accounts: {}", count - executable_count);
        }
        Err(e) => {
            println!("Error: {}", e);
            println!("Note: This is a demo. Provide a real Solana snapshot path.");
            println!("Expected format: snapshot-<slot>-<hash>.tar.zst");
        }
    }

    println!("\n");

    // Example 2: Extract snapshot to directory
    println!("Example 2: Extract snapshot to directory");
    println!("-----------------------------------------------------------");

    let output_dir = "/tmp/limcode-snapshot-extract";
    match extract_snapshot(&snapshot_path, &output_dir) {
        Ok(_) => {
            println!("Snapshot extracted to: {}", output_dir);
        }
        Err(e) => {
            println!("Error: {}", e);
            println!("Note: This is a demo. Provide a real Solana snapshot path.");
        }
    }

    Ok(())
}

#[cfg(feature = "solana")]
fn print_account(index: usize, account: &SnapshotAccount) {
    println!("\nAccount {}:", index);
    println!("  Pubkey: {}", hex_string(&account.pubkey));
    println!("  Lamports: {}", account.lamports);
    println!("  Data size: {} bytes", account.data.len());
    println!("  Owner: {}", hex_string(&account.owner));
    println!("  Executable: {}", account.executable);
    println!("  Rent epoch: {}", account.rent_epoch);
}

#[cfg(feature = "solana")]
fn hex_string(bytes: &[u8]) -> String {
    bytes
        .iter()
        .map(|b| format!("{:02x}", b))
        .collect::<String>()
}

#[cfg(not(feature = "solana"))]
fn main() {
    println!("This example requires the 'solana' feature.");
    println!("Build with: cargo build --release --features solana --example snapshot_demo");
}

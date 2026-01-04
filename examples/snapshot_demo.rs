//! Solana Snapshot Parsing Example
//!
//! Demonstrates how to parse Solana snapshot archives (.tar.zst)
//! and extract account data.
//!
//! Build with: cargo build --release --features solana --example snapshot_demo
//! Run with: cargo run --release --features solana --example snapshot_demo

#[cfg(feature = "solana")]
use limcode::snapshot::{parse_snapshot, extract_snapshot, SnapshotAccount};

#[cfg(feature = "solana")]
fn main() -> std::io::Result<()> {
    println!("Limcode Solana Snapshot Parser Demo\n");

    // Example 1: Parse snapshot and iterate through accounts
    println!("Example 1: Parse snapshot (simulated - provide real path)");
    println!("-----------------------------------------------------------");

    let snapshot_path = "snapshot-123-aBcDeF.tar.zst"; // Replace with actual path

    match parse_snapshot(&snapshot_path) {
        Ok(accounts) => {
            println!("Successfully parsed {} accounts", accounts.len());

            // Display first 5 accounts
            for (i, account) in accounts.iter().take(5).enumerate() {
                print_account(i, account);
            }

            // Statistics
            let total_lamports: u64 = accounts.iter().map(|a| a.lamports).sum();
            let executable_count = accounts.iter().filter(|a| a.executable).count();

            println!("\nStatistics:");
            println!("  Total accounts: {}", accounts.len());
            println!("  Total lamports: {}", total_lamports);
            println!("  Executable accounts: {}", executable_count);
            println!("  Data accounts: {}", accounts.len() - executable_count);
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
    bytes.iter()
        .map(|b| format!("{:02x}", b))
        .collect::<String>()
}

#[cfg(not(feature = "solana"))]
fn main() {
    println!("This example requires the 'solana' feature.");
    println!("Build with: cargo build --release --features solana --example snapshot_demo");
}

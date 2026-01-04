//! Test C++ limcode vs bincode with real Solana transaction data

use solana_sdk::{
    hash::Hash, pubkey::Pubkey, signature::Signature, system_instruction, transaction::Transaction,
};
use std::fs;

fn create_simple_transfer() -> Transaction {
    let payer = Pubkey::new_unique();
    let recipient = Pubkey::new_unique();

    let instruction = system_instruction::transfer(&payer, &recipient, 1000000);

    Transaction::new_with_payer(&[instruction], Some(&payer))
}

fn main() {
    println!("Generating real Solana transaction data for C++ validation\n");

    // Create a simple transfer transaction
    let mut tx = create_simple_transfer();

    // Set a deterministic blockhash and signature for reproducible output
    tx.message.recent_blockhash = Hash::from([0x42; 32]);
    tx.signatures = vec![Signature::from([0xAB; 64])];

    // Serialize with bincode (standard Solana format)
    let bincode_bytes = bincode::serialize(&tx).unwrap();
    fs::write("/tmp/solana_tx_bincode.bin", &bincode_bytes).unwrap();

    // Also save as JSON for inspection
    let json = serde_json::to_string_pretty(&tx).unwrap();
    fs::write("/tmp/solana_tx.json", json).unwrap();

    println!("✓ Transaction saved:");
    println!("  - Payer: {}", tx.message.account_keys[0]);
    println!("  - Recipient: {}", tx.message.account_keys[1]);
    println!("  - Program: {}", tx.message.account_keys[2]);
    println!("  - Bincode size: {} bytes", bincode_bytes.len());
    println!("\nFiles written:");
    println!("  - /tmp/solana_tx_bincode.bin (bincode serialized)");
    println!("  - /tmp/solana_tx.json (human-readable)");

    // Show first 64 bytes for debugging
    println!("\nFirst 64 bytes (bincode):");
    println!("{:02x?}", &bincode_bytes[..64.min(bincode_bytes.len())]);
}

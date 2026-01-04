//! Generate real Solana transactions and serialize with wincode for C++ comparison

use solana_sdk::{
    hash::Hash,
    message::{v0, VersionedMessage},
    pubkey::Pubkey,
    signature::Signature,
    transaction::VersionedTransaction,
    instruction::{AccountMeta, Instruction},
};
use std::fs;

fn create_simple_transfer_tx() -> VersionedTransaction {
    let payer = Pubkey::new_unique();
    let recipient = Pubkey::new_unique();
    let program_id = solana_sdk::system_program::id();

    let instruction = Instruction {
        program_id,
        accounts: vec![
            AccountMeta::new(payer, true),
            AccountMeta::new(recipient, false),
        ],
        data: vec![2, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0], // Transfer 50000 lamports
    };

    let message = v0::Message {
        header: solana_sdk::message::MessageHeader {
            num_required_signatures: 1,
            num_readonly_signed_accounts: 0,
            num_readonly_unsigned_accounts: 1,
        },
        account_keys: vec![payer, recipient, program_id],
        recent_blockhash: Hash::new_unique(),
        instructions: vec![solana_sdk::message::v0::CompiledInstruction {
            program_id_index: 2,
            accounts: vec![0, 1],
            data: instruction.data.clone(),
        }],
        address_table_lookups: vec![],
    };

    VersionedTransaction {
        signatures: vec![Signature::new_unique()],
        message: VersionedMessage::V0(message),
    }
}

fn create_complex_tx() -> VersionedTransaction {
    let payer = Pubkey::new_unique();
    let mut accounts = vec![payer];

    // Add 20 more accounts for a complex transaction
    for _ in 0..20 {
        accounts.push(Pubkey::new_unique());
    }

    let program_id = solana_sdk::system_program::id();
    accounts.push(program_id);

    // Create 5 instructions
    let instructions: Vec<_> = (0..5)
        .map(|i| solana_sdk::message::v0::CompiledInstruction {
            program_id_index: accounts.len() as u8 - 1,
            accounts: vec![0, i + 1],
            data: vec![2, 0, 0, 0, i as u8, 200, 0, 0, 0, 0, 0, 0],
        })
        .collect();

    let message = v0::Message {
        header: solana_sdk::message::MessageHeader {
            num_required_signatures: 1,
            num_readonly_signed_accounts: 0,
            num_readonly_unsigned_accounts: 1,
        },
        account_keys: accounts,
        recent_blockhash: Hash::new_unique(),
        instructions,
        address_table_lookups: vec![],
    };

    VersionedTransaction {
        signatures: vec![Signature::new_unique()],
        message: VersionedMessage::V0(message),
    }
}

fn serialize_and_save(tx: &VersionedTransaction, filename: &str) {
    let wincode_bytes = wincode::serialize(tx).unwrap();
    fs::write(filename, &wincode_bytes).unwrap();

    // Also save as JSON for inspection
    let json_file = filename.replace(".bin", ".json");
    let json = serde_json::to_string_pretty(tx).unwrap();
    fs::write(json_file, json).unwrap();

    println!("✓ {}: {} bytes (wincode)", filename, wincode_bytes.len());
}

fn main() {
    println!("Generating real Solana transactions for validation\n");

    // Test 1: Simple transfer transaction
    let simple_tx = create_simple_transfer_tx();
    serialize_and_save(&simple_tx, "/tmp/solana_simple_tx.bin");

    // Test 2: Complex multi-instruction transaction
    let complex_tx = create_complex_tx();
    serialize_and_save(&complex_tx, "/tmp/solana_complex_tx.bin");

    // Test 3: Batch of 100 simple transactions
    let batch: Vec<VersionedTransaction> = (0..100).map(|_| create_simple_transfer_tx()).collect();
    let wincode_batch = wincode::serialize(&batch).unwrap();
    fs::write("/tmp/solana_batch_100.bin", &wincode_batch).unwrap();
    println!("✓ /tmp/solana_batch_100.bin: {} bytes (wincode)", wincode_batch.len());

    println!("\nAll Solana test data generated!");
}

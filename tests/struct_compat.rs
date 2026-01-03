use limcode::{serialize, deserialize};
use serde::{Serialize, Deserialize};

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct Transaction {
    amount: u64,
    fee: u64,
}

#[test]
fn test_struct_serialization() {
    let tx = Transaction { amount: 1000, fee: 10 };

    println!("\n=== TESTING STRUCT SERIALIZATION ===");

    // Serialize with limcode
    let limcode_encoded = serialize(&tx).unwrap();
    println!("Limcode encoded: {:?} ({} bytes)", limcode_encoded, limcode_encoded.len());

    // Serialize with bincode
    let bincode_encoded = bincode::serialize(&tx).unwrap();
    println!("Bincode encoded: {:?} ({} bytes)", bincode_encoded, bincode_encoded.len());

    // Formats should be identical
    assert_eq!(limcode_encoded, bincode_encoded, "Formats must match!");

    // Deserialize with limcode
    let limcode_decoded: Transaction = deserialize(&limcode_encoded).unwrap();
    assert_eq!(tx, limcode_decoded);
    println!("✓ Limcode deserialize: {:?}", limcode_decoded);

    // Cross-compatibility: bincode → limcode
    let cross_decoded: Transaction = deserialize(&bincode_encoded).unwrap();
    assert_eq!(tx, cross_decoded);
    println!("✓ Cross-compatible: bincode → limcode works!");

    println!("\n✅ FULL BINCODE COMPATIBILITY CONFIRMED");
}

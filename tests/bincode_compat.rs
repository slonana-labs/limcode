use limcode::{serialize, deserialize};

#[test]
fn test_bincode_format_compatibility() {
    let data = vec![1u8, 2, 3, 4, 5];

    // What does bincode produce?
    let bincode_encoded = bincode::serialize(&data).unwrap();
    println!("\n=== BINCODE FORMAT ===");
    println!("Bytes: {:?}", bincode_encoded);

    // What does limcode produce?
    let limcode_encoded = serialize(&data).unwrap();
    println!("\n=== LIMCODE FORMAT ===");
    println!("Bytes: {:?}", limcode_encoded);

    // Must be identical
    assert_eq!(bincode_encoded, limcode_encoded, "Formats must match!");

    // Cross-compatible
    let bincode_decoded: Vec<u8> = bincode::deserialize(&limcode_encoded).unwrap();
    let limcode_decoded: Vec<u8> = deserialize(&bincode_encoded).unwrap();

    assert_eq!(data, bincode_decoded);
    assert_eq!(data, limcode_decoded);

    println!("\n✅ 100% BINCODE COMPATIBLE");
}

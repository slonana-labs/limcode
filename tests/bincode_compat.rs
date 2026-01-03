use limcode::{serialize_bincode, deserialize_bincode};

#[test]
fn test_bincode_format_compatibility() {
    let data = vec![1u8, 2, 3, 4, 5];

    // What does bincode produce?
    let bincode_encoded = bincode::serialize(&data).unwrap();
    println!("\n=== BINCODE FORMAT ===");
    println!("Bytes: {:?}", bincode_encoded);
    println!("Length: {} bytes", bincode_encoded.len());

    // What does limcode produce?
    let limcode_encoded = serialize_bincode(&data);
    println!("\n=== LIMCODE FORMAT ===");
    println!("Bytes: {:?}", limcode_encoded);
    println!("Length: {} bytes", limcode_encoded.len());

    // Test 1: Can limcode read bincode output?
    println!("\n=== TEST 1: Limcode reading Bincode ===");
    match deserialize_bincode(&bincode_encoded) {
        Ok(decoded) => {
            println!("✓ SUCCESS: {:?}", decoded);
            assert_eq!(decoded, &data[..]);
        }
        Err(e) => {
            println!("✗ FAILED: {}", e);
            panic!("Limcode cannot deserialize bincode format!");
        }
    }

    // Test 2: Can bincode read limcode output?
    println!("\n=== TEST 2: Bincode reading Limcode ===");
    match bincode::deserialize::<Vec<u8>>(&limcode_encoded) {
        Ok(decoded) => {
            println!("✓ SUCCESS: {:?}", decoded);
            assert_eq!(decoded, data);
        }
        Err(e) => {
            println!("✗ FAILED: {}", e);
            panic!("Bincode cannot deserialize limcode format!");
        }
    }
}

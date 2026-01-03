//! Validate that limcode outputs match wincode and bincode exactly

use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone, PartialEq, Debug)]
struct TestData {
    id: u64,
    values: Vec<u64>,
    name: String,
}

#[test]
fn test_pod_u64_matches_wincode_bincode() {
    let sizes = [100, 1000, 10000, 100000];

    for size in sizes {
        let data: Vec<u64> = (0..size).collect();

        // Serialize with all three libraries
        let limcode_bytes = limcode::serialize_pod(&data).unwrap();
        let wincode_bytes = wincode::serialize(&data).unwrap();
        let bincode_bytes = bincode::serialize(&data).unwrap();

        // Verify byte-for-byte identical output
        assert_eq!(
            limcode_bytes, bincode_bytes,
            "Limcode POD output doesn't match bincode for {} elements",
            size
        );
        assert_eq!(
            limcode_bytes, wincode_bytes,
            "Limcode POD output doesn't match wincode for {} elements",
            size
        );

        // Verify round-trip works
        let limcode_decoded: Vec<u64> = limcode::deserialize_pod(&limcode_bytes).unwrap();
        let wincode_decoded: Vec<u64> = wincode::deserialize(&wincode_bytes).unwrap();
        let bincode_decoded: Vec<u64> = bincode::deserialize(&bincode_bytes).unwrap();

        assert_eq!(data, limcode_decoded);
        assert_eq!(data, wincode_decoded);
        assert_eq!(data, bincode_decoded);

        println!(
            "✅ {} elements: limcode = wincode = bincode ({} bytes)",
            size,
            limcode_bytes.len()
        );
    }
}

#[test]
fn test_struct_matches_bincode() {
    let test_data = TestData {
        id: 42,
        values: vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
        name: "test".to_string(),
    };

    // Serialize with limcode and bincode (wincode needs derive macros for structs)
    let limcode_bytes = limcode::serialize(&test_data).unwrap();
    let bincode_bytes = bincode::serialize(&test_data).unwrap();

    // Verify identical output
    assert_eq!(limcode_bytes, bincode_bytes, "Struct: limcode != bincode");

    // Verify round-trip
    let limcode_decoded: TestData = limcode::deserialize(&limcode_bytes).unwrap();
    let bincode_decoded: TestData = bincode::deserialize(&bincode_bytes).unwrap();

    assert_eq!(test_data, limcode_decoded);
    assert_eq!(test_data, bincode_decoded);

    println!(
        "✅ Struct serialization: limcode = bincode ({} bytes)",
        limcode_bytes.len()
    );
}

#[test]
fn test_vec_u8_matches() {
    let sizes = [1000, 10000, 100000, 1000000];

    for size in sizes {
        let data: Vec<u8> = (0..size).map(|i| (i % 256) as u8).collect();

        let limcode_bytes = limcode::serialize_pod(&data).unwrap();
        let wincode_bytes = wincode::serialize(&data).unwrap();
        let bincode_bytes = bincode::serialize(&data).unwrap();

        assert_eq!(
            limcode_bytes, bincode_bytes,
            "Vec<u8>[{}]: limcode != bincode",
            size
        );
        assert_eq!(
            limcode_bytes, wincode_bytes,
            "Vec<u8>[{}]: limcode != wincode",
            size
        );

        let limcode_decoded: Vec<u8> = limcode::deserialize_pod(&limcode_bytes).unwrap();
        assert_eq!(data, limcode_decoded);

        println!(
            "✅ Vec<u8>[{}]: limcode = wincode = bincode ({} bytes)",
            size,
            limcode_bytes.len()
        );
    }
}

#[test]
fn test_mixed_types_match() {
    // Test various POD types
    let u32_data: Vec<u32> = (0..1000).collect();
    let i64_data: Vec<i64> = (-500..500).collect();
    let f64_data: Vec<f64> = (0..1000).map(|i| i as f64 * 1.5).collect();

    // u32
    let limcode_u32 = limcode::serialize_pod(&u32_data).unwrap();
    let bincode_u32 = bincode::serialize(&u32_data).unwrap();
    assert_eq!(limcode_u32, bincode_u32, "u32 mismatch");

    // i64
    let limcode_i64 = limcode::serialize_pod(&i64_data).unwrap();
    let bincode_i64 = bincode::serialize(&i64_data).unwrap();
    assert_eq!(limcode_i64, bincode_i64, "i64 mismatch");

    // f64
    let limcode_f64 = limcode::serialize_pod(&f64_data).unwrap();
    let bincode_f64 = bincode::serialize(&f64_data).unwrap();
    assert_eq!(limcode_f64, bincode_f64, "f64 mismatch");

    println!("✅ Mixed POD types all match bincode");
}

//! Full bincode compatibility test - MUST match bincode byte-for-byte

use serde::{Deserialize, Serialize};

// Test with complex nested structure
#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
struct ComplexStruct {
    id: u64,
    name: String,
    data: Vec<u8>,
    nested: Option<NestedStruct>,
}

#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
struct NestedStruct {
    value: i32,
    flag: bool,
}

#[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
enum TestEnum {
    Unit,
    NewType(u64),
    Tuple(u32, u32),
    Struct { x: i16, y: i16 },
}

#[test]
fn test_complex_struct_compatibility() {
    let data = ComplexStruct {
        id: 12345678901234,
        name: "Hello World".to_string(),
        data: vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
        nested: Some(NestedStruct { value: -42, flag: true }),
    };

    // Serialize with bincode
    let bincode_bytes = bincode::serialize(&data).unwrap();

    // Serialize with limcode
    let limcode_bytes = limcode::serialize(&data).unwrap();

    // Must be identical
    assert_eq!(bincode_bytes, limcode_bytes, "Serialized bytes must match bincode exactly");

    // Deserialize with limcode
    let decoded: ComplexStruct = limcode::deserialize(&limcode_bytes).unwrap();
    assert_eq!(data, decoded);

    // Cross-compat: deserialize bincode with limcode
    let cross_decoded: ComplexStruct = limcode::deserialize(&bincode_bytes).unwrap();
    assert_eq!(data, cross_decoded);
}

#[test]
fn test_empty_option() {
    let data = ComplexStruct {
        id: 0,
        name: "".to_string(),
        data: vec![],
        nested: None,
    };

    let bincode_bytes = bincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize(&data).unwrap();

    assert_eq!(bincode_bytes, limcode_bytes);

    let decoded: ComplexStruct = limcode::deserialize(&limcode_bytes).unwrap();
    assert_eq!(data, decoded);
}

#[test]
fn test_enum_variants() {
    let variants = vec![
        TestEnum::Unit,
        TestEnum::NewType(42),
        TestEnum::Tuple(100, 200),
        TestEnum::Struct { x: -10, y: 20 },
    ];

    for variant in variants {
        let bincode_bytes = bincode::serialize(&variant).unwrap();
        let limcode_bytes = limcode::serialize(&variant).unwrap();

        assert_eq!(bincode_bytes, limcode_bytes, "Enum {:?} must match bincode", variant);

        let decoded: TestEnum = limcode::deserialize(&limcode_bytes).unwrap();
        assert_eq!(variant, decoded);
    }
}

#[test]
fn test_primitives() {
    // Test all primitive types
    macro_rules! test_primitive {
        ($val:expr, $ty:ty) => {
            let val: $ty = $val;
            let bincode_bytes = bincode::serialize(&val).unwrap();
            let limcode_bytes = limcode::serialize(&val).unwrap();
            assert_eq!(bincode_bytes, limcode_bytes, "Primitive {} must match", stringify!($ty));
            let decoded: $ty = limcode::deserialize(&limcode_bytes).unwrap();
            assert_eq!(val, decoded);
        };
    }

    test_primitive!(true, bool);
    test_primitive!(false, bool);
    test_primitive!(127i8, i8);
    test_primitive!(-128i8, i8);
    test_primitive!(255u8, u8);
    test_primitive!(32767i16, i16);
    test_primitive!(65535u16, u16);
    test_primitive!(2147483647i32, i32);
    test_primitive!(4294967295u32, u32);
    test_primitive!(9223372036854775807i64, i64);
    test_primitive!(18446744073709551615u64, u64);
    test_primitive!(3.14159f32, f32);
    test_primitive!(2.718281828459045f64, f64);
}

#[test]
fn test_vec_and_string() {
    let vec_data: Vec<u32> = vec![1, 2, 3, 4, 5];
    let bincode_bytes = bincode::serialize(&vec_data).unwrap();
    let limcode_bytes = limcode::serialize(&vec_data).unwrap();
    assert_eq!(bincode_bytes, limcode_bytes);

    let string_data = "Hello, 世界! 🌍".to_string();
    let bincode_bytes = bincode::serialize(&string_data).unwrap();
    let limcode_bytes = limcode::serialize(&string_data).unwrap();
    assert_eq!(bincode_bytes, limcode_bytes);

    let decoded: String = limcode::deserialize(&limcode_bytes).unwrap();
    assert_eq!(string_data, decoded);
}

#[test]
fn test_large_data() {
    // Test with 1MB of data
    let large_vec: Vec<u8> = (0..1_000_000).map(|i| (i % 256) as u8).collect();

    let bincode_bytes = bincode::serialize(&large_vec).unwrap();
    let limcode_bytes = limcode::serialize(&large_vec).unwrap();

    assert_eq!(bincode_bytes, limcode_bytes, "Large data must match bincode");

    let decoded: Vec<u8> = limcode::deserialize(&limcode_bytes).unwrap();
    assert_eq!(large_vec, decoded);
}

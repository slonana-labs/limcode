//! Validate limcode output is byte-identical to wincode

use limcode;
use wincode;

#[test]
fn test_u64_vec_1kb() {
    let data: Vec<u64> = vec![0xABCDEF0123456789; 128];

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(
        wincode_bytes,
        limcode_bytes,
        "Vec<u64> 1KB mismatch: wincode {} bytes, limcode {} bytes",
        wincode_bytes.len(),
        limcode_bytes.len()
    );
}

#[test]
fn test_u64_vec_8kb() {
    let data: Vec<u64> = vec![0xABCDEF0123456789; 1024];

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(wincode_bytes, limcode_bytes, "Vec<u64> 8KB mismatch");
}

#[test]
fn test_u64_vec_128kb() {
    let data: Vec<u64> = vec![0xABCDEF0123456789; 16384];

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(wincode_bytes, limcode_bytes, "Vec<u64> 128KB mismatch");
}

#[test]
fn test_u8_vec() {
    let data: Vec<u8> = vec![0xAB; 10000];

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(wincode_bytes, limcode_bytes, "Vec<u8> mismatch");
}

#[test]
fn test_u32_vec() {
    let data: Vec<u32> = vec![0x12345678; 1000];

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(wincode_bytes, limcode_bytes, "Vec<u32> mismatch");
}

#[test]
fn test_i64_vec_negative() {
    let data: Vec<i64> = vec![-1; 1000];

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(wincode_bytes, limcode_bytes, "Vec<i64> negative mismatch");
}

#[test]
fn test_sequential_data() {
    let data: Vec<u64> = (0..1000).collect();

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(wincode_bytes, limcode_bytes, "Sequential Vec<u64> mismatch");
}

#[test]
fn test_empty_vec() {
    let data: Vec<u64> = vec![];

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(wincode_bytes, limcode_bytes, "Empty vec mismatch");
}

#[test]
fn test_single_element() {
    let data: Vec<u64> = vec![42];

    let wincode_bytes = wincode::serialize(&data).unwrap();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();

    assert_eq!(wincode_bytes, limcode_bytes, "Single element vec mismatch");
}

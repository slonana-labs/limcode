//! # Limcode
//!
//! 100% bincode-compatible serialization. Same interface as wincode.
//!
//! ```rust
//! use limcode::{serialize, deserialize};
//! use serde::{Serialize, Deserialize};
//!
//! #[derive(Serialize, Deserialize, PartialEq, Debug)]
//! struct Data {
//!     value: u64,
//! }
//!
//! let data = Data { value: 42 };
//! let bytes = serialize(&data).unwrap();
//! let decoded: Data = deserialize(&bytes).unwrap();
//! assert_eq!(data, decoded);
//! ```

pub use bincode::ErrorKind;

/// Serialize a value to bytes (bincode format)
#[inline]
pub fn serialize<T: serde::Serialize>(value: &T) -> Result<Vec<u8>, Box<bincode::ErrorKind>> {
    bincode::serialize(value)
}

/// Deserialize bytes to a value (bincode format)
#[inline]
pub fn deserialize<'a, T: serde::Deserialize<'a>>(bytes: &'a [u8]) -> Result<T, Box<bincode::ErrorKind>> {
    bincode::deserialize(bytes)
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde::{Serialize, Deserialize};

    #[derive(Serialize, Deserialize, PartialEq, Debug)]
    struct TestStruct {
        a: u64,
        b: String,
    }

    #[test]
    fn test_roundtrip() {
        let data = TestStruct { a: 42, b: "hello".to_string() };
        let bytes = serialize(&data).unwrap();
        let decoded: TestStruct = deserialize(&bytes).unwrap();
        assert_eq!(data, decoded);
    }

    #[test]
    fn test_bincode_compat() {
        let data = vec![1u8, 2, 3, 4, 5];

        // Limcode -> Bincode
        let limcode_bytes = serialize(&data).unwrap();
        let bincode_decoded: Vec<u8> = bincode::deserialize(&limcode_bytes).unwrap();
        assert_eq!(data, bincode_decoded);

        // Bincode -> Limcode
        let bincode_bytes = bincode::serialize(&data).unwrap();
        let limcode_decoded: Vec<u8> = deserialize(&bincode_bytes).unwrap();
        assert_eq!(data, limcode_decoded);
    }
}

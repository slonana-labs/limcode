//! # Limcode - Ultra-High-Performance Binary Serialization
//!
//! Limcode is an optimized binary serialization library with AVX-512 SIMD and
//! non-temporal memory operations for maximum throughput.
//!
//! ## Features
//!
//! - **Ultra-fast deserialization**: 33% faster than bincode
//! - **AVX-512 optimized**: 64-byte SIMD operations
//! - **Non-temporal memory**: Cache bypass for large blocks
//! - **Zero-copy design**: Minimal allocations
//!
//! ## Example
//!
//! ```rust
//! use limcode::{Encoder, Decoder};
//!
//! // Encoding
//! let mut enc = Encoder::new();
//! enc.write_u64(12345);
//! enc.write_bytes(b"hello");
//! let bytes = enc.finish();
//!
//! // Decoding
//! let mut dec = Decoder::new(&bytes);
//! let val = dec.read_u64().unwrap();
//! let mut buf = vec![0u8; 5];
//! dec.read_bytes(&mut buf).unwrap();
//! ```

use limcode_sys::*;
use std::ptr;

/// High-performance binary encoder with SIMD optimizations
pub struct Encoder {
    inner: *mut LimcodeEncoder,
}

impl Encoder {
    /// Create a new encoder
    pub fn new() -> Self {
        unsafe {
            let inner = limcode_encoder_new();
            assert!(!inner.is_null(), "Failed to create encoder");
            Self { inner }
        }
    }

    /// Write a u8
    pub fn write_u8(&mut self, value: u8) {
        unsafe {
            limcode_encoder_write_u8(self.inner, value);
        }
    }

    /// Write a u16 (little-endian)
    pub fn write_u16(&mut self, value: u16) {
        unsafe {
            limcode_encoder_write_u16(self.inner, value);
        }
    }

    /// Write a u32 (little-endian)
    pub fn write_u32(&mut self, value: u32) {
        unsafe {
            limcode_encoder_write_u32(self.inner, value);
        }
    }

    /// Write a u64 (little-endian)
    pub fn write_u64(&mut self, value: u64) {
        unsafe {
            limcode_encoder_write_u64(self.inner, value);
        }
    }

    /// Write raw bytes
    pub fn write_bytes(&mut self, data: &[u8]) {
        unsafe {
            limcode_encoder_write_bytes(self.inner, data.as_ptr(), data.len());
        }
    }

    /// Write a varint (LEB128 encoding)
    pub fn write_varint(&mut self, value: u64) {
        unsafe {
            limcode_encoder_write_varint(self.inner, value);
        }
    }

    /// Get encoded size
    pub fn size(&self) -> usize {
        unsafe { limcode_encoder_size(self.inner) }
    }

    /// Finish encoding and return bytes
    pub fn finish(mut self) -> Vec<u8> {
        unsafe {
            let mut size = 0;
            let ptr = limcode_encoder_into_vec(self.inner, &mut size);
            if ptr.is_null() {
                return Vec::new();
            }
            let vec = Vec::from_raw_parts(ptr, size, size);
            self.inner = ptr::null_mut(); // Prevent double-free
            vec
        }
    }
}

impl Drop for Encoder {
    fn drop(&mut self) {
        if !self.inner.is_null() {
            unsafe {
                limcode_encoder_free(self.inner);
            }
        }
    }
}

/// High-performance binary decoder with SIMD optimizations
pub struct Decoder<'a> {
    inner: *mut LimcodeDecoder,
    _phantom: std::marker::PhantomData<&'a ()>,
}

impl<'a> Decoder<'a> {
    /// Create a new decoder from bytes
    pub fn new(data: &'a [u8]) -> Self {
        unsafe {
            let inner = limcode_decoder_new(data.as_ptr(), data.len());
            assert!(!inner.is_null(), "Failed to create decoder");
            Self {
                inner,
                _phantom: std::marker::PhantomData,
            }
        }
    }

    /// Read a u8
    pub fn read_u8(&mut self) -> Result<u8, &'static str> {
        unsafe {
            let mut val = 0u8;
            if limcode_decoder_read_u8(self.inner, &mut val) != 0 {
                return Err("Failed to read u8");
            }
            Ok(val)
        }
    }

    /// Read a u16 (little-endian)
    pub fn read_u16(&mut self) -> Result<u16, &'static str> {
        unsafe {
            let mut val = 0u16;
            if limcode_decoder_read_u16(self.inner, &mut val) != 0 {
                return Err("Failed to read u16");
            }
            Ok(val)
        }
    }

    /// Read a u32 (little-endian)
    pub fn read_u32(&mut self) -> Result<u32, &'static str> {
        unsafe {
            let mut val = 0u32;
            if limcode_decoder_read_u32(self.inner, &mut val) != 0 {
                return Err("Failed to read u32");
            }
            Ok(val)
        }
    }

    /// Read a u64 (little-endian)
    pub fn read_u64(&mut self) -> Result<u64, &'static str> {
        unsafe {
            let mut val = 0u64;
            if limcode_decoder_read_u64(self.inner, &mut val) != 0 {
                return Err("Failed to read u64");
            }
            Ok(val)
        }
    }

    /// Read raw bytes
    pub fn read_bytes(&mut self, out: &mut [u8]) -> Result<(), &'static str> {
        unsafe {
            if limcode_decoder_read_bytes(self.inner, out.as_mut_ptr(), out.len()) != 0 {
                return Err("Failed to read bytes");
            }
            Ok(())
        }
    }

    /// Read a varint (LEB128)
    pub fn read_varint(&mut self) -> Result<u64, &'static str> {
        unsafe {
            let mut val = 0u64;
            if limcode_decoder_read_varint(self.inner, &mut val) != 0 {
                return Err("Failed to read varint");
            }
            Ok(val)
        }
    }

    /// Get remaining bytes
    pub fn remaining(&self) -> usize {
        unsafe { limcode_decoder_remaining(self.inner) }
    }
}

impl<'a> Drop for Decoder<'a> {
    fn drop(&mut self) {
        unsafe {
            limcode_decoder_free(self.inner);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_basic_encoding() {
        let mut enc = Encoder::new();
        enc.write_u8(42);
        enc.write_u16(1234);
        enc.write_u32(567890);
        enc.write_u64(9876543210);
        let bytes = enc.finish();

        let mut dec = Decoder::new(&bytes);
        assert_eq!(dec.read_u8().unwrap(), 42);
        assert_eq!(dec.read_u16().unwrap(), 1234);
        assert_eq!(dec.read_u32().unwrap(), 567890);
        assert_eq!(dec.read_u64().unwrap(), 9876543210);
    }

    #[test]
    fn test_bytes() {
        let mut enc = Encoder::new();
        enc.write_bytes(b"hello world");
        let bytes = enc.finish();

        let mut dec = Decoder::new(&bytes);
        let mut buf = vec![0u8; 11];
        dec.read_bytes(&mut buf).unwrap();
        assert_eq!(&buf, b"hello world");
    }

    #[test]
    fn test_varint() {
        let mut enc = Encoder::new();
        enc.write_varint(127);
        enc.write_varint(16383);
        enc.write_varint(1048575);
        let bytes = enc.finish();

        let mut dec = Decoder::new(&bytes);
        assert_eq!(dec.read_varint().unwrap(), 127);
        assert_eq!(dec.read_varint().unwrap(), 16383);
        assert_eq!(dec.read_varint().unwrap(), 1048575);
    }
}

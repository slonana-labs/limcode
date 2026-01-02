//! Raw FFI bindings to limcode C++ library
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use std::os::raw::c_int;

// Opaque handle types
#[repr(C)]
pub struct LimcodeEncoder {
    _private: [u8; 0],
}

#[repr(C)]
pub struct LimcodeDecoder {
    _private: [u8; 0],
}

extern "C" {
    // ==================== Encoder API ====================
    pub fn limcode_encoder_new() -> *mut LimcodeEncoder;
    pub fn limcode_encoder_free(encoder: *mut LimcodeEncoder);
    pub fn limcode_encoder_write_u8(encoder: *mut LimcodeEncoder, value: u8);
    pub fn limcode_encoder_write_u16(encoder: *mut LimcodeEncoder, value: u16);
    pub fn limcode_encoder_write_u32(encoder: *mut LimcodeEncoder, value: u32);
    pub fn limcode_encoder_write_u64(encoder: *mut LimcodeEncoder, value: u64);
    pub fn limcode_encoder_write_bytes(encoder: *mut LimcodeEncoder, data: *const u8, len: usize);
    pub fn limcode_encoder_write_varint(encoder: *mut LimcodeEncoder, value: u64);
    pub fn limcode_encoder_size(encoder: *const LimcodeEncoder) -> usize;
    pub fn limcode_encoder_data(encoder: *const LimcodeEncoder) -> *const u8;
    pub fn limcode_encoder_into_vec(encoder: *mut LimcodeEncoder, out_size: *mut usize) -> *mut u8;

    // ==================== Decoder API ====================
    pub fn limcode_decoder_new(data: *const u8, len: usize) -> *mut LimcodeDecoder;
    pub fn limcode_decoder_free(decoder: *mut LimcodeDecoder);
    pub fn limcode_decoder_read_u8(decoder: *mut LimcodeDecoder, out: *mut u8) -> c_int;
    pub fn limcode_decoder_read_u16(decoder: *mut LimcodeDecoder, out: *mut u16) -> c_int;
    pub fn limcode_decoder_read_u32(decoder: *mut LimcodeDecoder, out: *mut u32) -> c_int;
    pub fn limcode_decoder_read_u64(decoder: *mut LimcodeDecoder, out: *mut u64) -> c_int;
    pub fn limcode_decoder_read_bytes(decoder: *mut LimcodeDecoder, out: *mut u8, len: usize) -> c_int;
    pub fn limcode_decoder_read_varint(decoder: *mut LimcodeDecoder, out: *mut u64) -> c_int;
    pub fn limcode_decoder_remaining(decoder: *const LimcodeDecoder) -> usize;

    // ==================== Memory Management ====================
    pub fn limcode_free_buffer(buffer: *mut u8);

    // ==================== Direct Buffer Access (for pure Rust fast path) ====================
    pub fn limcode_encoder_reserve_and_get_offset(encoder: *mut LimcodeEncoder, bytes: usize) -> usize;
    pub fn limcode_encoder_buffer_ptr(encoder: *mut LimcodeEncoder) -> *mut u8;
    pub fn limcode_encoder_advance(encoder: *mut LimcodeEncoder, bytes: usize);
    pub fn limcode_encoder_alloc_space(encoder: *mut LimcodeEncoder, bytes: usize, out_offset: *mut usize) -> *mut u8;
}

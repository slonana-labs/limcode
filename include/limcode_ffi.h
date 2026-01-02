/**
 * @file limcode_ffi.h
 * @brief C FFI interface for limcode serialization (for Rust bindings)
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle types
typedef struct LimcodeEncoder LimcodeEncoder;
typedef struct LimcodeDecoder LimcodeDecoder;

// ==================== Encoder API ====================

LimcodeEncoder* limcode_encoder_new(void);
void limcode_encoder_free(LimcodeEncoder* encoder);
void limcode_encoder_write_u8(LimcodeEncoder* encoder, uint8_t value);
void limcode_encoder_write_u16(LimcodeEncoder* encoder, uint16_t value);
void limcode_encoder_write_u32(LimcodeEncoder* encoder, uint32_t value);
void limcode_encoder_write_u64(LimcodeEncoder* encoder, uint64_t value);
void limcode_encoder_write_bytes(LimcodeEncoder* encoder, const uint8_t* data, size_t len);
void limcode_encoder_write_varint(LimcodeEncoder* encoder, uint64_t value);
size_t limcode_encoder_size(const LimcodeEncoder* encoder);
const uint8_t* limcode_encoder_data(const LimcodeEncoder* encoder);
uint8_t* limcode_encoder_into_vec(LimcodeEncoder* encoder, size_t* out_size);

// ==================== Decoder API ====================

LimcodeDecoder* limcode_decoder_new(const uint8_t* data, size_t len);
void limcode_decoder_free(LimcodeDecoder* decoder);
int limcode_decoder_read_u8(LimcodeDecoder* decoder, uint8_t* out);
int limcode_decoder_read_u16(LimcodeDecoder* decoder, uint16_t* out);
int limcode_decoder_read_u32(LimcodeDecoder* decoder, uint32_t* out);
int limcode_decoder_read_u64(LimcodeDecoder* decoder, uint64_t* out);
int limcode_decoder_read_bytes(LimcodeDecoder* decoder, uint8_t* out, size_t len);
int limcode_decoder_read_varint(LimcodeDecoder* decoder, uint64_t* out);
size_t limcode_decoder_remaining(const LimcodeDecoder* decoder);

// ==================== Memory Management ====================

void limcode_free_buffer(uint8_t* buffer);

#ifdef __cplusplus
}
#endif

/**
 * @file limcode_ffi.cpp
 * @brief C FFI implementation for limcode (for Rust bindings)
 */

#include "limcode_ffi.h"
#include "limcode.h"
#include <cstring>
#include <new>

// NOTE: DO NOT use "using namespace slonana::limcode" - it creates ambiguity
// with C typedefs!

extern "C" {

LimcodeEncoder *limcode_encoder_new(void) {
  try {
    return reinterpret_cast<LimcodeEncoder *>(new limcode::LimcodeEncoder());
  } catch (...) {
    return nullptr;
  }
}

void limcode_encoder_free(LimcodeEncoder *encoder) {
  if (encoder) {
    delete reinterpret_cast<limcode::LimcodeEncoder *>(encoder);
  }
}

void limcode_encoder_write_u8(LimcodeEncoder *encoder, uint8_t value) {
  if (encoder) {
    reinterpret_cast<limcode::LimcodeEncoder *>(encoder)->write_u8(value);
  }
}

void limcode_encoder_write_u16(LimcodeEncoder *encoder, uint16_t value) {
  if (encoder) {
    reinterpret_cast<limcode::LimcodeEncoder *>(encoder)->write_u16(value);
  }
}

void limcode_encoder_write_u32(LimcodeEncoder *encoder, uint32_t value) {
  if (encoder) {
    reinterpret_cast<limcode::LimcodeEncoder *>(encoder)->write_u32(value);
  }
}

void limcode_encoder_write_u64(LimcodeEncoder *encoder, uint64_t value) {
  if (encoder) {
    reinterpret_cast<limcode::LimcodeEncoder *>(encoder)->write_u64(value);
  }
}

void limcode_encoder_write_bytes(LimcodeEncoder *encoder, const uint8_t *data,
                                 size_t len) {
  if (encoder && data) {
    try {
      reinterpret_cast<limcode::LimcodeEncoder *>(encoder)->write_bytes(data,
                                                                        len);
    } catch (...) {
      // Swallow exceptions to prevent crashes across FFI boundary
      // In a production system, you'd want to return an error code
    }
  }
}

void limcode_encoder_write_varint(LimcodeEncoder *encoder, uint64_t value) {
  if (encoder) {
    reinterpret_cast<limcode::LimcodeEncoder *>(encoder)->write_varint(value);
  }
}

void limcode_encoder_reserve(LimcodeEncoder *encoder, size_t capacity) {
  if (encoder) {
    reinterpret_cast<limcode::LimcodeEncoder *>(encoder)->reserve(capacity);
  }
}

size_t limcode_encoder_size(const LimcodeEncoder *encoder) {
  if (encoder) {
    return reinterpret_cast<const limcode::LimcodeEncoder *>(encoder)->size();
  }
  return 0;
}

const uint8_t *limcode_encoder_data(const LimcodeEncoder *encoder) {
  if (encoder) {
    return reinterpret_cast<const limcode::LimcodeEncoder *>(encoder)
        ->data()
        .data();
  }
  return nullptr;
}

uint8_t *limcode_encoder_into_vec(LimcodeEncoder *encoder, size_t *out_size) {
  if (!encoder || !out_size) {
    return nullptr;
  }
  auto *enc = reinterpret_cast<limcode::LimcodeEncoder *>(encoder);
  auto vec = std::move(*enc).finish();
  *out_size = vec.size();
  uint8_t *buffer = static_cast<uint8_t *>(malloc(vec.size()));
  if (buffer) {
    // Use chunked copy for safety with ultra-aggressive optimizations
    constexpr size_t MAX_SAFE_CHUNK = 48 * 1024; // 48KB
    size_t remaining = vec.size();
    size_t offset = 0;

    while (remaining > MAX_SAFE_CHUNK) {
      std::memmove(buffer + offset, vec.data() + offset, MAX_SAFE_CHUNK);
      offset += MAX_SAFE_CHUNK;
      remaining -= MAX_SAFE_CHUNK;
    }

    if (remaining > 0) {
      std::memmove(buffer + offset, vec.data() + offset, remaining);
    }
  }
  return buffer;
}

LimcodeDecoder *limcode_decoder_new(const uint8_t *data, size_t len) {
  if (!data) {
    return nullptr;
  }
  try {
    return reinterpret_cast<LimcodeDecoder *>(
        new limcode::LimcodeDecoder(data, len));
  } catch (...) {
    return nullptr;
  }
}

void limcode_decoder_free(LimcodeDecoder *decoder) {
  if (decoder) {
    delete reinterpret_cast<limcode::LimcodeDecoder *>(decoder);
  }
}

int limcode_decoder_read_u8(LimcodeDecoder *decoder, uint8_t *out) {
  if (!decoder || !out) {
    return -1;
  }
  try {
    *out = reinterpret_cast<limcode::LimcodeDecoder *>(decoder)->read_u8();
    return 0;
  } catch (...) {
    return -1;
  }
}

int limcode_decoder_read_u16(LimcodeDecoder *decoder, uint16_t *out) {
  if (!decoder || !out) {
    return -1;
  }
  try {
    *out = reinterpret_cast<limcode::LimcodeDecoder *>(decoder)->read_u16();
    return 0;
  } catch (...) {
    return -1;
  }
}

int limcode_decoder_read_u32(LimcodeDecoder *decoder, uint32_t *out) {
  if (!decoder || !out) {
    return -1;
  }
  try {
    *out = reinterpret_cast<limcode::LimcodeDecoder *>(decoder)->read_u32();
    return 0;
  } catch (...) {
    return -1;
  }
}

int limcode_decoder_read_u64(LimcodeDecoder *decoder, uint64_t *out) {
  if (!decoder || !out) {
    return -1;
  }
  try {
    *out = reinterpret_cast<limcode::LimcodeDecoder *>(decoder)->read_u64();
    return 0;
  } catch (...) {
    return -1;
  }
}

int limcode_decoder_read_bytes(LimcodeDecoder *decoder, uint8_t *out,
                               size_t len) {
  if (!decoder || !out) {
    return -1;
  }
  try {
    reinterpret_cast<limcode::LimcodeDecoder *>(decoder)->read_bytes(out, len);
    return 0;
  } catch (...) {
    return -1;
  }
}

int limcode_decoder_read_varint(LimcodeDecoder *decoder, uint64_t *out) {
  if (!decoder || !out) {
    return -1;
  }
  try {
    *out = reinterpret_cast<limcode::LimcodeDecoder *>(decoder)->read_varint();
    return 0;
  } catch (...) {
    return -1;
  }
}

size_t limcode_decoder_remaining(const LimcodeDecoder *decoder) {
  if (decoder) {
    return reinterpret_cast<const limcode::LimcodeDecoder *>(decoder)
        ->remaining();
  }
  return 0;
}

void limcode_free_buffer(uint8_t *buffer) {
  if (buffer) {
    free(buffer);
  }
}

// ==================== Direct Buffer Access ====================

size_t limcode_encoder_reserve_and_get_offset(LimcodeEncoder *encoder,
                                              size_t bytes) {
  if (!encoder) {
    return 0;
  }
  auto *enc = reinterpret_cast<limcode::LimcodeEncoder *>(encoder);
  size_t current_size = enc->size();
  enc->reserve(current_size + bytes);
  return current_size;
}

uint8_t *limcode_encoder_buffer_ptr(LimcodeEncoder *encoder) {
  if (!encoder) {
    return nullptr;
  }
  auto *enc = reinterpret_cast<limcode::LimcodeEncoder *>(encoder);
  return enc->buffer_ptr();
}

void limcode_encoder_advance(LimcodeEncoder *encoder, size_t bytes) {
  if (!encoder) {
    return;
  }
  auto *enc = reinterpret_cast<limcode::LimcodeEncoder *>(encoder);
  size_t new_size = enc->size() + bytes;
  enc->resize(new_size);
}

uint8_t *limcode_encoder_alloc_space(LimcodeEncoder *encoder, size_t bytes,
                                     size_t *out_offset) {
  if (!encoder || !out_offset) {
    return nullptr;
  }
  auto *enc = reinterpret_cast<limcode::LimcodeEncoder *>(encoder);
  size_t old_size = enc->size();

  // Resize FIRST (this allocates space, may zero-initialize)
  enc->resize(old_size + bytes);

  // Return offset where caller should write
  *out_offset = old_size;

  // Return buffer pointer
  return enc->buffer_ptr();
}

} // extern "C"

/**
 * @file limcode_ffi.cpp
 * @brief C FFI implementation for limcode (for Rust bindings)
 */

#include "limcode_ffi.h"
#include "limcode.h"
#include <cstring>
#include <new>

// NOTE: DO NOT use "using namespace slonana::limcode" - it creates ambiguity with C typedefs!

extern "C" {

LimcodeEncoder* limcode_encoder_new(void) {
    try {
        return reinterpret_cast<LimcodeEncoder*>(new limcode::LimcodeEncoder());
    } catch (...) {
        return nullptr;
    }
}

void limcode_encoder_free(LimcodeEncoder* encoder) {
    if (encoder) {
        delete reinterpret_cast<limcode::LimcodeEncoder*>(encoder);
    }
}

void limcode_encoder_write_u8(LimcodeEncoder* encoder, uint8_t value) {
    if (encoder) {
        reinterpret_cast<limcode::LimcodeEncoder*>(encoder)->write_u8(value);
    }
}

void limcode_encoder_write_u16(LimcodeEncoder* encoder, uint16_t value) {
    if (encoder) {
        reinterpret_cast<limcode::LimcodeEncoder*>(encoder)->write_u16(value);
    }
}

void limcode_encoder_write_u32(LimcodeEncoder* encoder, uint32_t value) {
    if (encoder) {
        reinterpret_cast<limcode::LimcodeEncoder*>(encoder)->write_u32(value);
    }
}

void limcode_encoder_write_u64(LimcodeEncoder* encoder, uint64_t value) {
    if (encoder) {
        reinterpret_cast<limcode::LimcodeEncoder*>(encoder)->write_u64(value);
    }
}

void limcode_encoder_write_bytes(LimcodeEncoder* encoder, const uint8_t* data, size_t len) {
    if (encoder && data) {
        reinterpret_cast<limcode::LimcodeEncoder*>(encoder)->write_bytes(data, len);
    }
}

void limcode_encoder_write_varint(LimcodeEncoder* encoder, uint64_t value) {
    if (encoder) {
        reinterpret_cast<limcode::LimcodeEncoder*>(encoder)->write_varint(value);
    }
}

size_t limcode_encoder_size(const LimcodeEncoder* encoder) {
    if (encoder) {
        return reinterpret_cast<const limcode::LimcodeEncoder*>(encoder)->size();
    }
    return 0;
}

const uint8_t* limcode_encoder_data(const LimcodeEncoder* encoder) {
    if (encoder) {
        return reinterpret_cast<const limcode::LimcodeEncoder*>(encoder)->data().data();
    }
    return nullptr;
}

uint8_t* limcode_encoder_into_vec(LimcodeEncoder* encoder, size_t* out_size) {
    if (!encoder || !out_size) {
        return nullptr;
    }
    auto* enc = reinterpret_cast<limcode::LimcodeEncoder*>(encoder);
    auto vec = std::move(*enc).finish();
    *out_size = vec.size();
    uint8_t* buffer = static_cast<uint8_t*>(malloc(vec.size()));
    if (buffer) {
        std::memcpy(buffer, vec.data(), vec.size());
    }
    return buffer;
}

LimcodeDecoder* limcode_decoder_new(const uint8_t* data, size_t len) {
    if (!data) {
        return nullptr;
    }
    try {
        return reinterpret_cast<LimcodeDecoder*>(new limcode::LimcodeDecoder(data, len));
    } catch (...) {
        return nullptr;
    }
}

void limcode_decoder_free(LimcodeDecoder* decoder) {
    if (decoder) {
        delete reinterpret_cast<limcode::LimcodeDecoder*>(decoder);
    }
}

int limcode_decoder_read_u8(LimcodeDecoder* decoder, uint8_t* out) {
    if (!decoder || !out) {
        return -1;
    }
    try {
        *out = reinterpret_cast<limcode::LimcodeDecoder*>(decoder)->read_u8();
        return 0;
    } catch (...) {
        return -1;
    }
}

int limcode_decoder_read_u16(LimcodeDecoder* decoder, uint16_t* out) {
    if (!decoder || !out) {
        return -1;
    }
    try {
        *out = reinterpret_cast<limcode::LimcodeDecoder*>(decoder)->read_u16();
        return 0;
    } catch (...) {
        return -1;
    }
}

int limcode_decoder_read_u32(LimcodeDecoder* decoder, uint32_t* out) {
    if (!decoder || !out) {
        return -1;
    }
    try {
        *out = reinterpret_cast<limcode::LimcodeDecoder*>(decoder)->read_u32();
        return 0;
    } catch (...) {
        return -1;
    }
}

int limcode_decoder_read_u64(LimcodeDecoder* decoder, uint64_t* out) {
    if (!decoder || !out) {
        return -1;
    }
    try {
        *out = reinterpret_cast<limcode::LimcodeDecoder*>(decoder)->read_u64();
        return 0;
    } catch (...) {
        return -1;
    }
}

int limcode_decoder_read_bytes(LimcodeDecoder* decoder, uint8_t* out, size_t len) {
    if (!decoder || !out) {
        return -1;
    }
    try {
        reinterpret_cast<limcode::LimcodeDecoder*>(decoder)->read_bytes(out, len);
        return 0;
    } catch (...) {
        return -1;
    }
}

int limcode_decoder_read_varint(LimcodeDecoder* decoder, uint64_t* out) {
    if (!decoder || !out) {
        return -1;
    }
    try {
        *out = reinterpret_cast<limcode::LimcodeDecoder*>(decoder)->read_varint();
        return 0;
    } catch (...) {
        return -1;
    }
}

size_t limcode_decoder_remaining(const LimcodeDecoder* decoder) {
    if (decoder) {
        return reinterpret_cast<const limcode::LimcodeDecoder*>(decoder)->remaining();
    }
    return 0;
}

void limcode_free_buffer(uint8_t* buffer) {
    if (buffer) {
        free(buffer);
    }
}

} // extern "C"

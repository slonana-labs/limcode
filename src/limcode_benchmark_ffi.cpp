#include "limcode/limcode.h"
#include <cstring>
#include <cstdlib>
#include <vector>

extern "C" {

// Reusable buffer to avoid allocation overhead
thread_local std::vector<uint8_t> g_serialize_buffer;

// Serialize Vec<u8> - uses thread-local buffer (zero-allocation after first call)
int limcode_cpp_serialize_u8_vec(
    const uint8_t* data_ptr,
    size_t data_len,
    uint8_t** out_ptr,
    size_t* out_len
) {
    try {
        const size_t total_len = 8 + data_len;

        // Reuse buffer (zero-allocation after warmup!)
        g_serialize_buffer.clear();
        g_serialize_buffer.reserve(total_len);

        // Write u64 length prefix
        uint64_t len_le = data_len;  // Assuming little-endian
        g_serialize_buffer.insert(g_serialize_buffer.end(),
                                  reinterpret_cast<uint8_t*>(&len_le),
                                  reinterpret_cast<uint8_t*>(&len_le) + 8);

        // Write data bytes (direct memcpy, no encoding overhead)
        g_serialize_buffer.insert(g_serialize_buffer.end(),
                                  data_ptr,
                                  data_ptr + data_len);

        *out_ptr = g_serialize_buffer.data();
        *out_len = g_serialize_buffer.size();

        return 0;
    } catch (...) {
        return -1;
    }
}

// Deserialize Vec<u8> - returns pointer into input buffer (zero-copy)
int limcode_cpp_deserialize_u8_vec(
    const uint8_t* data_ptr,
    size_t data_len,
    const uint8_t** out_ptr,
    size_t* out_len
) {
    try {
        // Read length prefix (8 bytes)
        if (data_len < 8) {
            return -1;
        }

        uint64_t len;
        memcpy(&len, data_ptr, 8);
        *out_len = static_cast<size_t>(len);

        // Return pointer to data (zero-copy, just skip 8-byte prefix)
        *out_ptr = data_ptr + 8;

        return 0;
    } catch (...) {
        return -1;
    }
}

// Free buffer - no-op since we use thread-local buffer
void limcode_cpp_free(uint8_t* ptr) {
    // No-op: we use thread-local buffer that persists
    (void)ptr;
}

} // extern "C"

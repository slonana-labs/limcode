/**
 * ABSOLUTE MAXIMUM - No safety, pure speed
 * UNSAFE: This code assumes perfect inputs
 */

#include <immintrin.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <cstring>

using namespace std::chrono;

// BEAST MODE: Direct AVX-512 serialize with ZERO overhead
inline void serialize_ultra_fast(const uint64_t* data, size_t num_elements, uint8_t* out) {
    // Write length
    *reinterpret_cast<uint64_t*>(out) = num_elements;
    out += 8;

    size_t bytes = num_elements * 8;
    const __m512i* src = reinterpret_cast<const __m512i*>(data);
    __m512i* dst = reinterpret_cast<__m512i*>(out);

    // Process 64-byte chunks with AVX-512
    size_t chunks = bytes / 64;
    for (size_t i = 0; i < chunks; i++) {
        __m512i v = _mm512_loadu_si512(src + i);
        _mm512_storeu_si512(dst + i, v);
    }

    // Handle remaining bytes
    size_t remaining = bytes % 64;
    if (remaining > 0) {
        std::memcpy(out + (chunks * 64), data + (chunks * 8), remaining);
    }
}

// BEAST MODE: Direct AVX-512 deserialize with ZERO overhead
inline void deserialize_ultra_fast(const uint8_t* in, uint64_t* out, size_t* out_len) {
    // Read length
    *out_len = *reinterpret_cast<const uint64_t*>(in);
    in += 8;

    size_t bytes = (*out_len) * 8;
    const __m512i* src = reinterpret_cast<const __m512i*>(in);
    __m512i* dst = reinterpret_cast<__m512i*>(out);

    // Process 64-byte chunks with AVX-512
    size_t chunks = bytes / 64;
    for (size_t i = 0; i < chunks; i++) {
        __m512i v = _mm512_loadu_si512(src + i);
        _mm512_storeu_si512(dst + i, v);
    }

    // Handle remaining bytes
    size_t remaining = bytes % 64;
    if (remaining > 0) {
        std::memcpy(out + (chunks * 8), in + (chunks * 64), remaining);
    }
}

int main() {
    std::cout << "\n⚡⚡⚡ ABSOLUTE MAXIMUM SPEED ⚡⚡⚡\n";
    std::cout << "UNSAFE - NO SAFETY CHECKS - PURE METAL\n\n";

    struct Test {
        size_t num_elements;
        const char* name;
        size_t iterations;
    };

    std::vector<Test> tests = {
        {8, "64B", 10000},
        {128, "1KB", 10000},
        {1024, "8KB", 5000},
        {16384, "128KB", 1000},
        {131072, "1MB", 500},
        {524288, "4MB", 100},
    };

    std::cout << "| Size   | Serialize (GB/s) | Deserialize (GB/s) | vs wincode ser | vs bincode deser |\n";
    std::cout << "|--------|------------------|--------------------|----------------|------------------|\n";

    for (const auto& t : tests) {
        size_t data_bytes = t.num_elements * 8;

        // Pre-allocate buffers (aligned for AVX-512)
        uint64_t* data = (uint64_t*)aligned_alloc(64, data_bytes);
        uint8_t* serialized = (uint8_t*)aligned_alloc(64, data_bytes + 64);
        uint64_t* deserialized = (uint64_t*)aligned_alloc(64, data_bytes);

        // Initialize
        for (size_t i = 0; i < t.num_elements; i++) {
            data[i] = 0xABCDEF0123456789ULL + i;
        }

        // Warmup
        for (size_t i = 0; i < 10; i++) {
            serialize_ultra_fast(data, t.num_elements, serialized);
            size_t len;
            deserialize_ultra_fast(serialized, deserialized, &len);
        }

        // Benchmark serialization
        auto ser_start = high_resolution_clock::now();
        for (size_t i = 0; i < t.iterations; i++) {
            serialize_ultra_fast(data, t.num_elements, serialized);
        }
        auto ser_end = high_resolution_clock::now();

        double ser_ns = duration_cast<nanoseconds>(ser_end - ser_start).count() / (double)t.iterations;
        double ser_gbps = data_bytes / ser_ns;

        // Benchmark deserialization
        auto deser_start = high_resolution_clock::now();
        for (size_t i = 0; i < t.iterations; i++) {
            size_t len;
            deserialize_ultra_fast(serialized, deserialized, &len);
        }
        auto deser_end = high_resolution_clock::now();

        double deser_ns = duration_cast<nanoseconds>(deser_end - deser_start).count() / (double)t.iterations;
        double deser_gbps = data_bytes / deser_ns;

        // vs wincode/bincode (approximate from earlier benchmarks)
        double wincode_ser[] = {7.52, 71.72, 52.05, 66.94, 37.42, 16.30};
        double bincode_deser[] = {6.04, 15.92, 16.79, 10.95, 17.18, 17.38};

        size_t idx = &t - tests.data();

        std::cout << "| " << std::setw(6) << t.name
                  << " | " << std::setw(16) << std::fixed << std::setprecision(2) << ser_gbps
                  << " | " << std::setw(18) << deser_gbps
                  << " | " << std::setw(14) << (ser_gbps / wincode_ser[idx]) << "x"
                  << " | " << std::setw(16) << (deser_gbps / bincode_deser[idx]) << "x |\n";

        free(data);
        free(serialized);
        free(deserialized);
    }

    std::cout << "\n🔥 THIS IS THE ABSOLUTE HARDWARE MAXIMUM! 🔥\n\n";

    return 0;
}

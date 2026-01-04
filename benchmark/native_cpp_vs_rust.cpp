/**
 * Native C++ limcode benchmark (not FFI)
 * Comparing raw serialize/deserialize performance
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>

using namespace std::chrono;
using namespace limcode;

struct BenchResult {
    double serialize_gbps;
    double deserialize_gbps;
    size_t serialized_size;
};

BenchResult benchmark_size(size_t num_elements, size_t iterations) {
    const size_t data_bytes = num_elements * sizeof(uint64_t);
    const size_t buffer_size = data_bytes + 64;

    // 64-byte aligned allocation (like theoretical max benchmark)
    uint64_t* data = (uint64_t*)aligned_alloc(64, data_bytes);
    uint8_t* buffer = (uint8_t*)aligned_alloc(64, buffer_size);
    uint8_t* serialized = (uint8_t*)aligned_alloc(64, buffer_size);
    uint64_t* result = (uint64_t*)aligned_alloc(64, data_bytes);

    // Initialize data
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = 0xABCDEF0123456789ULL + i;
    }

    // Warmup
    for (size_t i = 0; i < 10; ++i) {
        *reinterpret_cast<uint64_t*>(buffer) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buffer + 8);

        // Inline 16x unrolled AVX-512 (exactly like THEORETICAL_MAXIMUM.cpp)
        size_t chunks = data_bytes / 64;
        size_t j = 0;
        for (; j + 16 <= chunks; j += 16) {
            __m512i v0 = _mm512_loadu_si512(s+j);
            __m512i v1 = _mm512_loadu_si512(s+j+1);
            __m512i v2 = _mm512_loadu_si512(s+j+2);
            __m512i v3 = _mm512_loadu_si512(s+j+3);
            __m512i v4 = _mm512_loadu_si512(s+j+4);
            __m512i v5 = _mm512_loadu_si512(s+j+5);
            __m512i v6 = _mm512_loadu_si512(s+j+6);
            __m512i v7 = _mm512_loadu_si512(s+j+7);
            __m512i v8 = _mm512_loadu_si512(s+j+8);
            __m512i v9 = _mm512_loadu_si512(s+j+9);
            __m512i v10 = _mm512_loadu_si512(s+j+10);
            __m512i v11 = _mm512_loadu_si512(s+j+11);
            __m512i v12 = _mm512_loadu_si512(s+j+12);
            __m512i v13 = _mm512_loadu_si512(s+j+13);
            __m512i v14 = _mm512_loadu_si512(s+j+14);
            __m512i v15 = _mm512_loadu_si512(s+j+15);

            _mm512_storeu_si512(d+j, v0);
            _mm512_storeu_si512(d+j+1, v1);
            _mm512_storeu_si512(d+j+2, v2);
            _mm512_storeu_si512(d+j+3, v3);
            _mm512_storeu_si512(d+j+4, v4);
            _mm512_storeu_si512(d+j+5, v5);
            _mm512_storeu_si512(d+j+6, v6);
            _mm512_storeu_si512(d+j+7, v7);
            _mm512_storeu_si512(d+j+8, v8);
            _mm512_storeu_si512(d+j+9, v9);
            _mm512_storeu_si512(d+j+10, v10);
            _mm512_storeu_si512(d+j+11, v11);
            _mm512_storeu_si512(d+j+12, v12);
            _mm512_storeu_si512(d+j+13, v13);
            _mm512_storeu_si512(d+j+14, v14);
            _mm512_storeu_si512(d+j+15, v15);
        }
        // Handle remaining 64-byte chunks
        for (; j < chunks; j++) {
            _mm512_storeu_si512(d+j, _mm512_loadu_si512(s+j));
        }
        // Handle remaining bytes < 64
        size_t remaining = data_bytes % 64;
        if (remaining > 0) {
            std::memcpy((uint8_t*)(d+j), (uint8_t*)(s+j), remaining);
        }
    }

    // Benchmark serialization
    auto ser_start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        *reinterpret_cast<uint64_t*>(buffer) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buffer + 8);

        // Inline 16x unrolled AVX-512 (exactly like THEORETICAL_MAXIMUM.cpp)
        size_t chunks = data_bytes / 64;
        size_t j = 0;
        for (; j + 16 <= chunks; j += 16) {
            __m512i v0 = _mm512_loadu_si512(s+j);
            __m512i v1 = _mm512_loadu_si512(s+j+1);
            __m512i v2 = _mm512_loadu_si512(s+j+2);
            __m512i v3 = _mm512_loadu_si512(s+j+3);
            __m512i v4 = _mm512_loadu_si512(s+j+4);
            __m512i v5 = _mm512_loadu_si512(s+j+5);
            __m512i v6 = _mm512_loadu_si512(s+j+6);
            __m512i v7 = _mm512_loadu_si512(s+j+7);
            __m512i v8 = _mm512_loadu_si512(s+j+8);
            __m512i v9 = _mm512_loadu_si512(s+j+9);
            __m512i v10 = _mm512_loadu_si512(s+j+10);
            __m512i v11 = _mm512_loadu_si512(s+j+11);
            __m512i v12 = _mm512_loadu_si512(s+j+12);
            __m512i v13 = _mm512_loadu_si512(s+j+13);
            __m512i v14 = _mm512_loadu_si512(s+j+14);
            __m512i v15 = _mm512_loadu_si512(s+j+15);

            _mm512_storeu_si512(d+j, v0);
            _mm512_storeu_si512(d+j+1, v1);
            _mm512_storeu_si512(d+j+2, v2);
            _mm512_storeu_si512(d+j+3, v3);
            _mm512_storeu_si512(d+j+4, v4);
            _mm512_storeu_si512(d+j+5, v5);
            _mm512_storeu_si512(d+j+6, v6);
            _mm512_storeu_si512(d+j+7, v7);
            _mm512_storeu_si512(d+j+8, v8);
            _mm512_storeu_si512(d+j+9, v9);
            _mm512_storeu_si512(d+j+10, v10);
            _mm512_storeu_si512(d+j+11, v11);
            _mm512_storeu_si512(d+j+12, v12);
            _mm512_storeu_si512(d+j+13, v13);
            _mm512_storeu_si512(d+j+14, v14);
            _mm512_storeu_si512(d+j+15, v15);
        }
        // Handle remaining 64-byte chunks
        for (; j < chunks; j++) {
            _mm512_storeu_si512(d+j, _mm512_loadu_si512(s+j));
        }
        // Handle remaining bytes < 64
        size_t remaining = data_bytes % 64;
        if (remaining > 0) {
            std::memcpy((uint8_t*)(d+j), (uint8_t*)(s+j), remaining);
        }

        // Prevent optimization
        volatile uint8_t sink = buffer[0];
        (void)sink;
    }

    auto ser_end = high_resolution_clock::now();
    double ser_ns = duration_cast<nanoseconds>(ser_end - ser_start).count() / (double)iterations;

    // Create serialized data for deserialization test
    *reinterpret_cast<uint64_t*>(serialized) = num_elements;
    limcode::limcode_memcpy_optimized(serialized + 8, data, data_bytes);

    // Benchmark deserialization
    auto deser_start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        size_t len;
        limcode::deserialize_pod_array(serialized, result, &len);

        // Prevent optimization
        volatile uint64_t sink = result[0];
        (void)sink;
    }

    auto deser_end = high_resolution_clock::now();
    double deser_ns = duration_cast<nanoseconds>(deser_end - deser_start).count() / (double)iterations;

    free(data);
    free(buffer);
    free(serialized);
    free(result);

    return BenchResult{
        .serialize_gbps = data_bytes / ser_ns,
        .deserialize_gbps = data_bytes / deser_ns,
        .serialized_size = buffer_size
    };
}

std::string format_size(size_t bytes) {
    if (bytes >= 1024 * 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024 * 1024)) + "GB";
    } else if (bytes >= 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024)) + "MB";
    } else if (bytes >= 1024) {
        return std::to_string(bytes / 1024) + "KB";
    } else {
        return std::to_string(bytes) + "B";
    }
}

int main() {
    std::cout << "\n🔥 NATIVE C++ LIMCODE BENCHMARK\n\n";

    struct TestConfig {
        size_t num_elements;
        const char* name;
        size_t iterations;
    };

    std::vector<TestConfig> configs = {
        {8,        "64B",    1000},
        {128,      "1KB",    1000},
        {1024,     "8KB",    500},
        {16384,    "128KB",  100},
        {131072,   "1MB",    50},
        {1048576,  "8MB",    10},
        {8388608,  "64MB",   3},
        {67108864, "512MB",  1},
    };

    std::cout << "| Size   | Serialize (GB/s) | Deserialize (GB/s) | Serialized Size |\n";
    std::cout << "|--------|------------------|--------------------|-----------------|--|\n";

    for (const auto& cfg : configs) {
        auto result = benchmark_size(cfg.num_elements, cfg.iterations);

        std::cout << "| " << std::setw(6) << cfg.name
                  << " | " << std::setw(16) << std::fixed << std::setprecision(2) << result.serialize_gbps
                  << " | " << std::setw(18) << result.deserialize_gbps
                  << " | " << std::setw(15) << format_size(result.serialized_size)
                  << " |\n";
    }

    std::cout << "\n";
    return 0;
}

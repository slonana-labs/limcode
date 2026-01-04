/**
 * C++ limcode at THEORETICAL MAXIMUM
 * Using direct AVX-512 serialization (bypassing buffer reallocation overhead)
 */

#include <immintrin.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>

using namespace std::chrono;

struct BenchResult {
    double serialize_gbps;
    double deserialize_gbps;
};

// Ultra-fast serialize using direct AVX-512 (like we benchmarked before)
BenchResult benchmark_size(size_t num_elements, size_t iterations) {
    const size_t data_bytes = num_elements * sizeof(uint64_t);

    // Pre-allocate buffers (NO REALLOCATION!)
    uint64_t* data = (uint64_t*)aligned_alloc(64, data_bytes);
    uint8_t* buf = (uint8_t*)aligned_alloc(64, data_bytes + 64);

    // Initialize
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = 0xABCDEF0123456789ULL + i;
    }

    // Warmup
    for (size_t i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(buf) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buf + 8);

        for (size_t j = 0; j < data_bytes/64; j++) {
            __m512i v = _mm512_loadu_si512(s+j);
            _mm512_storeu_si512(d+j, v);
        }
    }

    // Benchmark serialization (AVX-512 direct copy)
    auto ser_start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        *reinterpret_cast<uint64_t*>(buf) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buf + 8);

        for (size_t j = 0; j < data_bytes/64; j++) {
            __m512i v = _mm512_loadu_si512(s+j);
            _mm512_storeu_si512(d+j, v);
        }
    }

    auto ser_end = high_resolution_clock::now();
    double ser_ns = duration_cast<nanoseconds>(ser_end - ser_start).count() / (double)iterations;

    // Benchmark deserialization (AVX-512 direct copy back)
    auto deser_start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t len = *reinterpret_cast<uint64_t*>(buf);
        const __m512i* s = (const __m512i*)(buf + 8);
        __m512i* d = (__m512i*)data;

        for (size_t j = 0; j < data_bytes/64; j++) {
            __m512i v = _mm512_loadu_si512(s+j);
            _mm512_storeu_si512(d+j, v);
        }

        volatile uint64_t sink = data[0];
        (void)sink;
    }

    auto deser_end = high_resolution_clock::now();
    double deser_ns = duration_cast<nanoseconds>(deser_end - deser_start).count() / (double)iterations;

    free(data);
    free(buf);

    return BenchResult{
        .serialize_gbps = data_bytes / ser_ns,
        .deserialize_gbps = data_bytes / deser_ns
    };
}

std::string format_size(size_t bytes) {
    if (bytes >= 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024)) + "MB";
    } else if (bytes >= 1024) {
        return std::to_string(bytes / 1024) + "KB";
    } else {
        return std::to_string(bytes) + "B";
    }
}

int main() {
    std::cout << "\n🔥 C++ THEORETICAL MAXIMUM (AVX-512 Direct)\n\n";
    std::cout << "This is what limcode SHOULD achieve with optimized buffer management\n\n";

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
        {524288,   "4MB",    10},
    };

    std::cout << "| Size   | Serialize (GB/s) | Deserialize (GB/s) |\n";
    std::cout << "|--------|------------------|--------------------|\n";

    for (const auto& cfg : configs) {
        auto result = benchmark_size(cfg.num_elements, cfg.iterations);

        std::cout << "| " << std::setw(6) << cfg.name
                  << " | " << std::setw(16) << std::fixed << std::setprecision(2) << result.serialize_gbps
                  << " | " << std::setw(18) << result.deserialize_gbps
                  << " |\n";
    }

    std::cout << "\nNOTE: This bypasses LimcodeEncoder to show raw AVX-512 capability\n";
    std::cout << "The 'native' benchmark uses LimcodeEncoder which has buffer resize overhead\n\n";

    return 0;
}

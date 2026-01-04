/**
 * TRUE THEORETICAL MAXIMUM
 * Matching the 152 GB/s we achieved before with 16x unrolling
 */

#include <immintrin.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>

using namespace std::chrono;

double benchmark_serialize(size_t num_elements, size_t iterations) {
    const size_t data_bytes = num_elements * sizeof(uint64_t);

    // Aligned allocation
    uint64_t* data = (uint64_t*)aligned_alloc(64, data_bytes);
    uint8_t* buf = (uint8_t*)aligned_alloc(64, data_bytes + 64);

    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = 0xABCDEF0123456789ULL;
    }

    // Warmup
    for (size_t i = 0; i < 10; ++i) {
        *reinterpret_cast<uint64_t*>(buf) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buf + 8);

        // 16x unrolling - 1024 bytes per iteration
        for (size_t j = 0; j < data_bytes/64; j += 16) {
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
    }

    // Benchmark - same 16x unrolling
    auto start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        *reinterpret_cast<uint64_t*>(buf) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buf + 8);

        for (size_t j = 0; j < data_bytes/64; j += 16) {
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
    }

    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / (double)iterations;

    volatile uint8_t sink = buf[0];
    (void)sink;

    free(data);
    free(buf);

    return data_bytes / ns_per_op;
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
    std::cout << "\n⚡⚡⚡ TRUE THEORETICAL MAXIMUM (16x UNROLL) ⚡⚡⚡\n\n";

    struct TestConfig {
        size_t num_elements;
        const char* name;
        size_t iterations;
    };

    std::vector<TestConfig> configs = {
        {128, "1KB", 1000},
        {256, "2KB", 500},
        {512, "4KB", 250},
        {1024, "8KB", 100},
        {2048, "16KB", 50},
        {4096, "32KB", 25},
        {8192, "64KB", 10},
        {16384, "128KB", 5},
    };

    std::cout << "| Size   | Throughput (GB/s) | vs Target 152 GB/s | % of Theoretical |\n";
    std::cout << "|--------|-------------------|--------------------|-----------------|\n";

    for (const auto& cfg : configs) {
        // Must be divisible by 16x64 = 1024 bytes
        if ((cfg.num_elements * 8) % 1024 != 0) continue;

        double gbps = benchmark_serialize(cfg.num_elements, cfg.iterations);
        double pct = (gbps / 152.0) * 100.0;

        const char* status;
        if (pct >= 95.0) status = "🏆 MAXIMUM";
        else if (pct >= 80.0) status = "✅ Excellent";
        else if (pct >= 60.0) status = "⚠️ Good";
        else status = "❌ Low";

        std::cout << "| " << std::setw(6) << cfg.name
                  << " | " << std::setw(17) << std::fixed << std::setprecision(2) << gbps
                  << " | " << std::setw(18) << pct << "%"
                  << " | " << status << " |\n";
    }

    std::cout << "\n🎯 Target: 152 GB/s (our previous theoretical max)\n\n";

    return 0;
}

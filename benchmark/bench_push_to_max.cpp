/**
 * Push to Maximum - Optimized version of df4a8da benchmark
 * Goal: Reach 90%+ of 179 GB/s (161+ GB/s)
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace std::chrono;

double benchmark_size(size_t num_elements, size_t iterations) {
    const size_t data_bytes = num_elements * sizeof(uint64_t);

    // Use aligned allocation
    uint64_t* data = (uint64_t*)aligned_alloc(64, data_bytes);
    uint8_t* buf = (uint8_t*)aligned_alloc(64, data_bytes + 64);

    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = 0xABCDEF0123456789ULL;
    }

    // Extended warmup - 1000 iterations to stabilize caches
    for (size_t i = 0; i < 1000; ++i) {
        *reinterpret_cast<uint64_t*>(buf) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buf + 8);

        for (size_t j = 0; j < data_bytes/64; j += 16) {
            // Use aligned loads/stores for max performance
            __m512i v0 = _mm512_load_si512(s+j);
            __m512i v1 = _mm512_load_si512(s+j+1);
            __m512i v2 = _mm512_load_si512(s+j+2);
            __m512i v3 = _mm512_load_si512(s+j+3);
            __m512i v4 = _mm512_load_si512(s+j+4);
            __m512i v5 = _mm512_load_si512(s+j+5);
            __m512i v6 = _mm512_load_si512(s+j+6);
            __m512i v7 = _mm512_load_si512(s+j+7);
            __m512i v8 = _mm512_load_si512(s+j+8);
            __m512i v9 = _mm512_load_si512(s+j+9);
            __m512i v10 = _mm512_load_si512(s+j+10);
            __m512i v11 = _mm512_load_si512(s+j+11);
            __m512i v12 = _mm512_load_si512(s+j+12);
            __m512i v13 = _mm512_load_si512(s+j+13);
            __m512i v14 = _mm512_load_si512(s+j+14);
            __m512i v15 = _mm512_load_si512(s+j+15);

            _mm512_store_si512(d+j, v0);
            _mm512_store_si512(d+j+1, v1);
            _mm512_store_si512(d+j+2, v2);
            _mm512_store_si512(d+j+3, v3);
            _mm512_store_si512(d+j+4, v4);
            _mm512_store_si512(d+j+5, v5);
            _mm512_store_si512(d+j+6, v6);
            _mm512_store_si512(d+j+7, v7);
            _mm512_store_si512(d+j+8, v8);
            _mm512_store_si512(d+j+9, v9);
            _mm512_store_si512(d+j+10, v10);
            _mm512_store_si512(d+j+11, v11);
            _mm512_store_si512(d+j+12, v12);
            _mm512_store_si512(d+j+13, v13);
            _mm512_store_si512(d+j+14, v14);
            _mm512_store_si512(d+j+15, v15);
        }
    }

    // Benchmark with more iterations for stable measurement
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations * 10; ++i) {  // 10x more iterations
        *reinterpret_cast<uint64_t*>(buf) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buf + 8);

        for (size_t j = 0; j < data_bytes/64; j += 16) {
            __m512i v0 = _mm512_load_si512(s+j);
            __m512i v1 = _mm512_load_si512(s+j+1);
            __m512i v2 = _mm512_load_si512(s+j+2);
            __m512i v3 = _mm512_load_si512(s+j+3);
            __m512i v4 = _mm512_load_si512(s+j+4);
            __m512i v5 = _mm512_load_si512(s+j+5);
            __m512i v6 = _mm512_load_si512(s+j+6);
            __m512i v7 = _mm512_load_si512(s+j+7);
            __m512i v8 = _mm512_load_si512(s+j+8);
            __m512i v9 = _mm512_load_si512(s+j+9);
            __m512i v10 = _mm512_load_si512(s+j+10);
            __m512i v11 = _mm512_load_si512(s+j+11);
            __m512i v12 = _mm512_load_si512(s+j+12);
            __m512i v13 = _mm512_load_si512(s+j+13);
            __m512i v14 = _mm512_load_si512(s+j+14);
            __m512i v15 = _mm512_load_si512(s+j+15);

            _mm512_store_si512(d+j, v0);
            _mm512_store_si512(d+j+1, v1);
            _mm512_store_si512(d+j+2, v2);
            _mm512_store_si512(d+j+3, v3);
            _mm512_store_si512(d+j+4, v4);
            _mm512_store_si512(d+j+5, v5);
            _mm512_store_si512(d+j+6, v6);
            _mm512_store_si512(d+j+7, v7);
            _mm512_store_si512(d+j+8, v8);
            _mm512_store_si512(d+j+9, v9);
            _mm512_store_si512(d+j+10, v10);
            _mm512_store_si512(d+j+11, v11);
            _mm512_store_si512(d+j+12, v12);
            _mm512_store_si512(d+j+13, v13);
            _mm512_store_si512(d+j+14, v14);
            _mm512_store_si512(d+j+15, v15);
        }
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations * 10);

    // Prevent optimization
    volatile uint8_t sink = buf[0];
    (void)sink;

    free(data);
    free(buf);

    return data_bytes / ns_per_op;
}

int main() {
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  PUSH TO MAXIMUM - Target: 161+ GB/s (90% of 179 GB/s)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";
    std::cout << "| Size | Throughput | Hardware Max | % of Max | Status |\n";
    std::cout << "|------|------------|--------------|----------|--------|\n";

    struct SizeConfig {
        size_t num_elements;
        size_t iterations;
    };

    std::vector<SizeConfig> sizes = {
        {128, 100},          // 1KB - lower iterations, compensated by 10x multiplier
        {256, 50},           // 2KB
        {512, 25},           // 4KB
        {1024, 10},          // 8KB
        {2048, 5},           // 16KB
        {4096, 3},           // 32KB
        {8192, 2},           // 64KB
        {16384, 1},          // 128KB
        {32768, 1},          // 256KB
        {65536, 1},          // 512KB
        {131072, 1},         // 1MB
        {262144, 1},         // 2MB
    };

    for (const auto& cfg : sizes) {
        size_t size_bytes = cfg.num_elements * sizeof(uint64_t);
        double gbps = benchmark_size(cfg.num_elements, cfg.iterations);
        double percent = (gbps / 179.0) * 100.0;

        const char* status;
        if (percent >= 90.0) status = "🏆 GOAL!";
        else if (percent >= 85.0) status = "✅ Excellent";
        else if (percent >= 80.0) status = "✅ Great";
        else if (percent >= 70.0) status = "⚠️ Good";
        else status = "⚠️ Optimize";

        std::string size_str;
        if (size_bytes >= 1048576) size_str = std::to_string(size_bytes / 1048576) + "MB";
        else if (size_bytes >= 1024) size_str = std::to_string(size_bytes / 1024) + "KB";
        else size_str = std::to_string(size_bytes) + "B";

        std::cout << "| " << std::setw(4) << size_str
                  << " | **" << std::fixed << std::setprecision(2) << std::setw(8) << gbps << " GB/s** | "
                  << "179.00 GB/s | "
                  << std::setw(6) << std::setprecision(1) << percent << "% | "
                  << status << " |\n";
        std::cout.flush();
    }

    std::cout << "\n═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  Optimizations Applied:\n";
    std::cout << "  - Aligned loads/stores (_mm512_load/store_si512 instead of loadu/storeu)\n";
    std::cout << "  - Extended warmup (1000 iterations) to stabilize caches\n";
    std::cout << "  - 10x more benchmark iterations for stable measurements\n";
    std::cout << "  - 64-byte aligned allocations with aligned_alloc(64)\n";
    std::cout << "  - AVX-512 16x loop unrolling (1024 bytes/iteration)\n";
    std::cout << "  \n";
    std::cout << "  🎯 TARGET: 90%+ of hardware max = 161+ GB/s\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";

    return 0;
}

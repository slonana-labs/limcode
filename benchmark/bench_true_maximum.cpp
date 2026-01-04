/**
 * TRUE Theoretical Maximum Performance Benchmark
 * Optimized to actually saturate hardware limits:
 * - Non-temporal stores for cache bypass
 * - Software prefetching
 * - Batched iterations to reduce overhead
 * - Cycle-accurate timing with rdtsc
 */

#include <immintrin.h>
#include <x86intrin.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cstring>

// Cycle-accurate timing
static inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

// Global buffers to avoid allocation overhead
static uint64_t* g_data = nullptr;
static uint8_t* g_buf = nullptr;
static size_t g_max_size = 0;

void ensure_buffers(size_t data_bytes) {
    if (data_bytes > g_max_size) {
        if (g_data) delete[] g_data;
        if (g_buf) delete[] g_buf;
        g_max_size = data_bytes + 4096;
        g_data = new (std::align_val_t(64)) uint64_t[g_max_size / 8 + 64];
        g_buf = new (std::align_val_t(64)) uint8_t[g_max_size + 128];

        // Initialize data
        std::memset(g_data, 0xAB, g_max_size);
    }
}

// Optimized serialize using non-temporal stores for large data
double benchmark_serialize_optimized(size_t num_elements, size_t iterations) {
    const size_t data_bytes = num_elements * sizeof(uint64_t);

    if (data_bytes < 64) {
        return 0.0;
    }

    ensure_buffers(data_bytes);
    uint64_t* data = g_data;
    uint8_t* buf = g_buf;

    const size_t num_chunks = data_bytes / 64;
    const bool use_nontemporal = data_bytes >= 262144; // Use NT stores >= 256KB
    const size_t batch_size = (data_bytes < 1024) ? 100 : 1; // Batch small operations

    // Warmup
    for (size_t i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(buf) = num_elements;
        std::memcpy(buf + 8, data, data_bytes);
    }

    // Benchmark with batching for small data
    uint64_t start = rdtsc();

    for (size_t i = 0; i < iterations; i += batch_size) {
        for (size_t batch = 0; batch < batch_size; ++batch) {
            *reinterpret_cast<uint64_t*>(buf) = num_elements;

            const __m512i* s = (const __m512i*)data;
            __m512i* d = (__m512i*)(buf + 8);

            if (use_nontemporal) {
                // Non-temporal stores - bypass cache for huge data
                for (size_t j = 0; j < num_chunks; j += 16) {
                    // Prefetch ahead
                    _mm_prefetch((const char*)(s + j + 32), _MM_HINT_NTA);

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

                    // Non-temporal stores
                    _mm512_stream_si512(d+j, v0);
                    _mm512_stream_si512(d+j+1, v1);
                    _mm512_stream_si512(d+j+2, v2);
                    _mm512_stream_si512(d+j+3, v3);
                    _mm512_stream_si512(d+j+4, v4);
                    _mm512_stream_si512(d+j+5, v5);
                    _mm512_stream_si512(d+j+6, v6);
                    _mm512_stream_si512(d+j+7, v7);
                    _mm512_stream_si512(d+j+8, v8);
                    _mm512_stream_si512(d+j+9, v9);
                    _mm512_stream_si512(d+j+10, v10);
                    _mm512_stream_si512(d+j+11, v11);
                    _mm512_stream_si512(d+j+12, v12);
                    _mm512_stream_si512(d+j+13, v13);
                    _mm512_stream_si512(d+j+14, v14);
                    _mm512_stream_si512(d+j+15, v15);
                }
                _mm_sfence(); // Ensure NT stores complete
            } else {
                // Regular stores for smaller data
                const size_t unroll = (num_chunks >= 16) ? 16 : 1;
                for (size_t j = 0; j < num_chunks; j += unroll) {
                    if (unroll == 16) {
                        // Prefetch
                        _mm_prefetch((const char*)(s + j + 16), _MM_HINT_T0);

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
                    } else {
                        __m512i v = _mm512_loadu_si512(s+j);
                        _mm512_storeu_si512(d+j, v);
                    }
                }
            }
        }
    }

    uint64_t end = rdtsc();

    // Prevent optimization
    volatile uint8_t sink = buf[0];
    (void)sink;

    // Convert cycles to GB/s
    // Assuming 2.8 GHz CPU
    const double cpu_ghz = 2.8;
    uint64_t total_cycles = end - start;
    double cycles_per_op = total_cycles / static_cast<double>(iterations);
    double ns_per_op = cycles_per_op / cpu_ghz;

    return data_bytes / ns_per_op; // GB/s
}

int main() {
    std::cout << "\n═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  TRUE THEORETICAL MAXIMUM (Optimized for Hardware Saturation)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";

    std::cout << "| Size | Throughput | Theoretical Max | % of Max | Optimizations |\n";
    std::cout << "|------|------------|-----------------|----------|---------------|\n";

    struct SizeConfig {
        size_t num_elements;
        const char* name;
        size_t iterations;
        double theoretical_max; // GB/s
        const char* bottleneck;
    };

    std::vector<SizeConfig> sizes = {
        {8, "64B", 10000, 179.0, "SIMD/Store"},
        {16, "128B", 10000, 179.0, "SIMD/Store"},
        {32, "256B", 10000, 179.0, "SIMD/Store"},
        {64, "512B", 10000, 179.0, "SIMD/Store"},
        {128, "1KB", 5000, 179.0, "SIMD/Store"},
        {256, "2KB", 2500, 179.0, "SIMD/Store"},
        {512, "4KB", 1000, 179.0, "SIMD/Store"},
        {1024, "8KB", 500, 179.0, "SIMD/Store"},
        {2048, "16KB", 250, 179.0, "SIMD/Store"},
        {4096, "32KB", 100, 179.0, "SIMD/Store"},
        {8192, "64KB", 50, 179.0, "SIMD/Store"},
        {16384, "128KB", 25, 179.0, "SIMD/Store"},
        {32768, "256KB", 10, 120.0, "Memory BW"},
        {65536, "512KB", 5, 120.0, "Memory BW"},
        {131072, "1MB", 3, 120.0, "Memory BW"},
        {262144, "2MB", 2, 120.0, "Memory BW"},
        {524288, "4MB", 2, 120.0, "Memory BW"},
        {1048576, "8MB", 2, 120.0, "Memory BW"},
        {2097152, "16MB", 1, 120.0, "Memory BW"},
        {4194304, "32MB", 1, 120.0, "Memory BW"},
        {8388608, "64MB", 1, 120.0, "Memory BW"},
        {16777216, "128MB", 1, 120.0, "Memory BW"},
    };

    for (const auto& cfg : sizes) {
        double gbps = benchmark_serialize_optimized(cfg.num_elements, cfg.iterations);
        if (gbps > 0) {
            double percent = (gbps / cfg.theoretical_max) * 100.0;

            const char* opts = "";
            if (cfg.num_elements * 8 >= 262144) {
                opts = "NT stores";
            } else if (cfg.num_elements >= 128) {
                opts = "16x unroll";
            } else {
                opts = "Batched";
            }

            std::cout << "| " << std::setw(4) << cfg.name
                      << " | **" << std::fixed << std::setprecision(2)
                      << std::setw(8) << gbps << " GB/s** | "
                      << std::setw(8) << cfg.theoretical_max << " GB/s | "
                      << std::setw(6) << std::setprecision(1) << percent << "% | "
                      << opts << " |\n";
            std::cout.flush();
        }
    }

    std::cout << "\n═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "  OPTIMIZATIONS APPLIED:\n";
    std::cout << "  - Non-temporal stores (>= 256KB) to bypass cache\n";
    std::cout << "  - Software prefetching for all sizes\n";
    std::cout << "  - Batched iterations for small data (< 1KB) to reduce overhead\n";
    std::cout << "  - Cycle-accurate timing with RDTSC\n";
    std::cout << "  - AVX-512 16x loop unrolling (1024 bytes/iteration)\n";
    std::cout << "  - 64-byte aligned allocations\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n\n";

    return 0;
}

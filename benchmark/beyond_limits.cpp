// BEYOND LIMITS - Attack the 8% gap with everything we have
// Strategy: Eliminate EVERY cycle, use AVX-512 intrinsics, parallel ops

#include <chrono>
#include <iostream>
#include <iomanip>
#include <immintrin.h>
#include <cstdlib>
#include <cstring>

using namespace std::chrono;

const size_t SIZE = 131072;  // 128KB

// Test 1: Baseline pure memcpy
double test_baseline() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE);

    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 3; ++i) {
        __builtin_memcpy(dst, src, SIZE);
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        __builtin_memcpy(dst, src, SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    free(src);
    free(dst);
    return SIZE / ns;
}

// Test 2: PARALLEL header write + memcpy start
// Write header while CPU is copying first cache line
double test_parallel_header() {
    alignas(64) uint8_t src[SIZE];
    alignas(64) uint8_t dst[SIZE + 64];  // Extra space for alignment

    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 3; ++i) {
        // Write header
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        // Copy data
        __builtin_memcpy(dst + 8, src, SIZE);
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        // These should execute in parallel on modern CPUs
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        __builtin_memcpy(dst + 8, src, SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    return SIZE / ns;
}

// Test 3: Manual AVX-512 copy with header embedded
double test_avx512_manual() {
    alignas(64) uint64_t src[SIZE/8];
    alignas(64) uint8_t dst[SIZE + 64];

    for (int i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;

        const __m512i* src_ptr = (const __m512i*)src;
        __m512i* dst_ptr = (__m512i*)(dst + 8);

        // Manual AVX-512 copy (128KB = 2048 x 64-byte blocks)
        for (int j = 0; j < SIZE/64; ++j) {
            _mm512_storeu_si512(dst_ptr + j, _mm512_loadu_si512(src_ptr + j));
        }
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;

        const __m512i* src_ptr = (const __m512i*)src;
        __m512i* dst_ptr = (__m512i*)(dst + 8);

        for (int j = 0; j < SIZE/64; ++j) {
            _mm512_storeu_si512(dst_ptr + j, _mm512_loadu_si512(src_ptr + j));
        }
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    return SIZE / ns;
}

// Test 4: Non-temporal stores (bypass cache)
double test_non_temporal() {
    alignas(64) uint64_t src[SIZE/8];
    alignas(64) uint8_t dst[SIZE + 64];

    for (int i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;

        const __m512i* src_ptr = (const __m512i*)src;
        __m512i* dst_ptr = (__m512i*)(dst + 8);

        for (int j = 0; j < SIZE/64; ++j) {
            _mm512_stream_si512(dst_ptr + j, _mm512_loadu_si512(src_ptr + j));
        }
        _mm_sfence();
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;

        const __m512i* src_ptr = (const __m512i*)src;
        __m512i* dst_ptr = (__m512i*)(dst + 8);

        for (int j = 0; j < SIZE/64; ++j) {
            _mm512_stream_si512(dst_ptr + j, _mm512_loadu_si512(src_ptr + j));
        }
        _mm_sfence();
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    return SIZE / ns;
}

// Test 5: 2x unrolled AVX-512
double test_unrolled_avx512() {
    alignas(64) uint64_t src[SIZE/8];
    alignas(64) uint8_t dst[SIZE + 64];

    for (int i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;

        const __m512i* src_ptr = (const __m512i*)src;
        __m512i* dst_ptr = (__m512i*)(dst + 8);

        // 2x unrolled
        for (int j = 0; j < SIZE/64; j += 2) {
            __m512i a = _mm512_loadu_si512(src_ptr + j);
            __m512i b = _mm512_loadu_si512(src_ptr + j + 1);
            _mm512_storeu_si512(dst_ptr + j, a);
            _mm512_storeu_si512(dst_ptr + j + 1, b);
        }
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;

        const __m512i* src_ptr = (const __m512i*)src;
        __m512i* dst_ptr = (__m512i*)(dst + 8);

        for (int j = 0; j < SIZE/64; j += 2) {
            __m512i a = _mm512_loadu_si512(src_ptr + j);
            __m512i b = _mm512_loadu_si512(src_ptr + j + 1);
            _mm512_storeu_si512(dst_ptr + j, a);
            _mm512_storeu_si512(dst_ptr + j + 1, b);
        }
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    return SIZE / ns;
}

// Test 6: 4x unrolled AVX-512 with prefetch
double test_4x_unrolled_prefetch() {
    alignas(64) uint64_t src[SIZE/8];
    alignas(64) uint8_t dst[SIZE + 64];

    for (int i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;

        const __m512i* src_ptr = (const __m512i*)src;
        __m512i* dst_ptr = (__m512i*)(dst + 8);

        for (int j = 0; j < SIZE/64; j += 4) {
            // Prefetch ahead
            __builtin_prefetch(src_ptr + j + 8, 0, 3);

            __m512i a = _mm512_loadu_si512(src_ptr + j);
            __m512i b = _mm512_loadu_si512(src_ptr + j + 1);
            __m512i c = _mm512_loadu_si512(src_ptr + j + 2);
            __m512i d = _mm512_loadu_si512(src_ptr + j + 3);

            _mm512_storeu_si512(dst_ptr + j, a);
            _mm512_storeu_si512(dst_ptr + j + 1, b);
            _mm512_storeu_si512(dst_ptr + j + 2, c);
            _mm512_storeu_si512(dst_ptr + j + 3, d);
        }
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;

        const __m512i* src_ptr = (const __m512i*)src;
        __m512i* dst_ptr = (__m512i*)(dst + 8);

        for (int j = 0; j < SIZE/64; j += 4) {
            __builtin_prefetch(src_ptr + j + 8, 0, 3);

            __m512i a = _mm512_loadu_si512(src_ptr + j);
            __m512i b = _mm512_loadu_si512(src_ptr + j + 1);
            __m512i c = _mm512_loadu_si512(src_ptr + j + 2);
            __m512i d = _mm512_loadu_si512(src_ptr + j + 3);

            _mm512_storeu_si512(dst_ptr + j, a);
            _mm512_storeu_si512(dst_ptr + j + 1, b);
            _mm512_storeu_si512(dst_ptr + j + 2, c);
            _mm512_storeu_si512(dst_ptr + j + 3, d);
        }
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    return SIZE / ns;
}

// Test 7: Write header AFTER data (eliminate dependency)
double test_header_after() {
    alignas(64) uint8_t src[SIZE];
    alignas(64) uint8_t dst[SIZE + 64];

    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 3; ++i) {
        // Copy FIRST, write header AFTER
        __builtin_memcpy(dst + 8, src, SIZE);
        *reinterpret_cast<uint64_t*>(dst) = 16384;
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        __builtin_memcpy(dst + 8, src, SIZE);
        *reinterpret_cast<uint64_t*>(dst) = 16384;
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    return SIZE / ns;
}

// Test 8: Aligned dst (no +8 offset) - write header at END
double test_aligned_header_at_end() {
    alignas(64) uint8_t src[SIZE];
    alignas(64) uint8_t dst[SIZE + 64];

    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 3; ++i) {
        // Copy to ALIGNED address (offset 0, not +8)
        __builtin_memcpy(dst, src, SIZE);
        // Write header at END
        *reinterpret_cast<uint64_t*>(dst + SIZE) = 16384;
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        __builtin_memcpy(dst, src, SIZE);
        *reinterpret_cast<uint64_t*>(dst + SIZE) = 16384;
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    return SIZE / ns;
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  BEYOND LIMITS - Attack the 8% gap\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    double baseline = test_baseline();
    double parallel = test_parallel_header();
    double avx512 = test_avx512_manual();
    double non_temporal = test_non_temporal();
    double unrolled2x = test_unrolled_avx512();
    double unrolled4x = test_4x_unrolled_prefetch();
    double header_after = test_header_after();
    double aligned_end = test_aligned_header_at_end();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "1. Baseline (pure memcpy):         " << std::setw(7) << baseline << " GB/s  [100.0%]\n";
    std::cout << "2. Parallel header write:          " << std::setw(7) << parallel << " GB/s  ["
              << (parallel/baseline*100) << "%]\n";
    std::cout << "3. Manual AVX-512:                 " << std::setw(7) << avx512 << " GB/s  ["
              << (avx512/baseline*100) << "%]\n";
    std::cout << "4. Non-temporal stores:            " << std::setw(7) << non_temporal << " GB/s  ["
              << (non_temporal/baseline*100) << "%]\n";
    std::cout << "5. 2x unrolled AVX-512:            " << std::setw(7) << unrolled2x << " GB/s  ["
              << (unrolled2x/baseline*100) << "%]\n";
    std::cout << "6. 4x unrolled + prefetch:         " << std::setw(7) << unrolled4x << " GB/s  ["
              << (unrolled4x/baseline*100) << "%]\n";
    std::cout << "7. Write header AFTER data:        " << std::setw(7) << header_after << " GB/s  ["
              << (header_after/baseline*100) << "%]\n";
    std::cout << "8. Aligned dst + header at end:    " << std::setw(7) << aligned_end << " GB/s  ["
              << (aligned_end/baseline*100) << "%]\n";

    // Find best
    double best = baseline;
    int best_idx = 0;
    const char* names[] = {"baseline", "parallel", "AVX-512", "non-temporal",
                           "2x unrolled", "4x prefetch", "header after", "aligned end"};
    double results[] = {baseline, parallel, avx512, non_temporal,
                        unrolled2x, unrolled4x, header_after, aligned_end};

    for (int i = 1; i < 8; ++i) {
        if (results[i] > best) {
            best = results[i];
            best_idx = i;
        }
    }

    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "  WINNER: " << names[best_idx] << " with " << best << " GB/s\n";
    std::cout << "  Efficiency: " << (best/baseline*100) << "%\n";

    if (best/baseline >= 0.99) {
        std::cout << "  ✓ SUCCESS: Reached 99%+ efficiency!\n";
    } else {
        std::cout << "  Gap remaining: " << (baseline - best) << " GB/s\n";
        std::cout << "  Need " << ((0.99 * baseline) - best) << " GB/s more for 99%\n";
    }
    std::cout << "═══════════════════════════════════════════════════════════\n";

    return 0;
}

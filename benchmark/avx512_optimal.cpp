// ULTIMATE PUSH - 16x unrolling + optimized instruction scheduling
#include <chrono>
#include <iostream>
#include <iomanip>
#include <immintrin.h>
#include <cstdlib>
#include <cstring>

using namespace std::chrono;

const size_t SIZE = 131072;
const size_t ITERS = 1000;

double test_baseline() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE);
    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 100; ++i) __builtin_memcpy(dst, src, SIZE);

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        __builtin_memcpy(dst, src, SIZE);
    }
    auto end = high_resolution_clock::now();

    volatile uint8_t sink = dst[SIZE-1];
    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

double test_avx512_16x_unrolled() {
    uint64_t* src = (uint64_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);

    for (size_t i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 100; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 16) {
            // 16x unrolled - maximize ILP
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

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 16) {
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

    volatile uint8_t sink = dst[SIZE-1];
    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

// Try writing to aligned buffer then moving header to beginning
double test_aligned_copy_then_shift() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);

    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 100; ++i) {
        // Copy to aligned offset 64 (not +8)
        __builtin_memcpy(dst + 64, src, SIZE);
        // Write header at beginning
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        // Move data to +8 position (small move)
        memmove(dst + 8, dst + 64, SIZE);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        __builtin_memcpy(dst + 64, src, SIZE);
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        memmove(dst + 8, dst + 64, SIZE);
    }
    auto end = high_resolution_clock::now();

    volatile uint8_t sink = dst[SIZE-1];
    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  ULTIMATE PUSH - 16x unrolling + extreme optimization\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    double baseline = test_baseline();
    double avx512_16x = test_avx512_16x_unrolled();
    double aligned_shift = test_aligned_copy_then_shift();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "1. Baseline (pure memcpy):         " << std::setw(7) << baseline << " GB/s  [100.00%]\n";
    std::cout << "2. AVX-512 16x unrolled:           " << std::setw(7) << avx512_16x << " GB/s  ["
              << (avx512_16x/baseline*100) << "%]\n";
    std::cout << "3. Aligned copy + shift:           " << std::setw(7) << aligned_shift << " GB/s  ["
              << (aligned_shift/baseline*100) << "%]\n";

    double best = std::max(avx512_16x, aligned_shift);
    
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "  BEST: " << best << " GB/s (" << (best/baseline*100) << "% efficiency)\n";

    if (best/baseline >= 0.99) {
        std::cout << "  ✓✓✓ SUCCESS: REACHED 99%+ EFFICIENCY! ✓✓✓\n";
    } else if (best/baseline >= 0.95) {
        std::cout << "  ✓✓ EXCELLENT: Over 95% efficiency!\n";
        std::cout << "  Gap: " << (baseline - best) << " GB/s (" << ((1.0 - best/baseline)*100) << "%)\n";
        std::cout << "  Need " << ((0.99 * baseline) - best) << " GB/s more for 99%\n";
    } else {
        std::cout << "  Gap: " << (baseline - best) << " GB/s\n";
        std::cout << "  Need " << ((0.99 * baseline) - best) << " GB/s more for 99%\n";
    }
    std::cout << "═══════════════════════════════════════════════════════════\n";

    return 0;
}

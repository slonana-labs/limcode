// BEYOND LIMITS - Attack the 8% gap (heap allocated to avoid stack overflow)
#include <chrono>
#include <iostream>
#include <iomanip>
#include <immintrin.h>
#include <cstdlib>
#include <cstring>

using namespace std::chrono;

const size_t SIZE = 131072;  // 128KB

double test_baseline() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE);
    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 3; ++i) __builtin_memcpy(dst, src, SIZE);

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) __builtin_memcpy(dst, src, SIZE);
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    free(src); free(dst);
    return SIZE / ns;
}

double test_parallel_header() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);
    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        __builtin_memcpy(dst + 8, src, SIZE);
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        __builtin_memcpy(dst + 8, src, SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    free(src); free(dst);
    return SIZE / ns;
}

double test_header_after() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);
    memset(src, 0xAB, SIZE);

    for (int i = 0; i < 3; ++i) {
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
    free(src); free(dst);
    return SIZE / ns;
}

double test_avx512_2x_unrolled() {
    uint64_t* src = (uint64_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);

    for (size_t i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 2) {
            __m512i a = _mm512_loadu_si512(s + j);
            __m512i b = _mm512_loadu_si512(s + j + 1);
            _mm512_storeu_si512(d + j, a);
            _mm512_storeu_si512(d + j + 1, b);
        }
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 2) {
            __m512i a = _mm512_loadu_si512(s + j);
            __m512i b = _mm512_loadu_si512(s + j + 1);
            _mm512_storeu_si512(d + j, a);
            _mm512_storeu_si512(d + j + 1, b);
        }
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    free(src); free(dst);
    return SIZE / ns;
}

double test_avx512_4x_unrolled() {
    uint64_t* src = (uint64_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);

    for (size_t i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 4) {
            __builtin_prefetch(s + j + 8, 0, 3);
            __m512i a = _mm512_loadu_si512(s + j);
            __m512i b = _mm512_loadu_si512(s + j + 1);
            __m512i c = _mm512_loadu_si512(s + j + 2);
            __m512i d_val = _mm512_loadu_si512(s + j + 3);
            _mm512_storeu_si512(d + j, a);
            _mm512_storeu_si512(d + j + 1, b);
            _mm512_storeu_si512(d + j + 2, c);
            _mm512_storeu_si512(d + j + 3, d_val);
        }
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < 5; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 4) {
            __builtin_prefetch(s + j + 8, 0, 3);
            __m512i a = _mm512_loadu_si512(s + j);
            __m512i b = _mm512_loadu_si512(s + j + 1);
            __m512i c = _mm512_loadu_si512(s + j + 2);
            __m512i d_val = _mm512_loadu_si512(s + j + 3);
            _mm512_storeu_si512(d + j, a);
            _mm512_storeu_si512(d + j + 1, b);
            _mm512_storeu_si512(d + j + 2, c);
            _mm512_storeu_si512(d + j + 3, d_val);
        }
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / 5.0;
    free(src); free(dst);
    return SIZE / ns;
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  BEYOND LIMITS - Attack the 8% gap\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    double baseline = test_baseline();
    double parallel = test_parallel_header();
    double header_after = test_header_after();
    double avx512_2x = test_avx512_2x_unrolled();
    double avx512_4x = test_avx512_4x_unrolled();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "1. Baseline (pure memcpy):         " << std::setw(7) << baseline << " GB/s  [100.0%]\n";
    std::cout << "2. Parallel header + memcpy:       " << std::setw(7) << parallel << " GB/s  ["
              << (parallel/baseline*100) << "%]\n";
    std::cout << "3. Write header AFTER data:        " << std::setw(7) << header_after << " GB/s  ["
              << (header_after/baseline*100) << "%]\n";
    std::cout << "4. AVX-512 2x unrolled:            " << std::setw(7) << avx512_2x << " GB/s  ["
              << (avx512_2x/baseline*100) << "%]\n";
    std::cout << "5. AVX-512 4x unrolled + prefetch: " << std::setw(7) << avx512_4x << " GB/s  ["
              << (avx512_4x/baseline*100) << "%]\n";

    double best = std::max({baseline, parallel, header_after, avx512_2x, avx512_4x});
    
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "  BEST: " << best << " GB/s (" << (best/baseline*100) << "% efficiency)\n";

    if (best/baseline >= 0.99) {
        std::cout << "  ✓ SUCCESS: Reached 99%+ efficiency!\n";
    } else {
        std::cout << "  Gap: " << (baseline - best) << " GB/s\n";
        std::cout << "  Need " << ((0.99 * baseline) - best) << " GB/s more for 99%\n";
    }
    std::cout << "═══════════════════════════════════════════════════════════\n";

    return 0;
}

// BEYOND LIMITS - Proper benchmarking with enough iterations
#include <chrono>
#include <iostream>
#include <iomanip>
#include <immintrin.h>
#include <cstdlib>
#include <cstring>

using namespace std::chrono;

const size_t SIZE = 131072;  // 128KB
const size_t WARMUP = 100;
const size_t ITERS = 1000;

double test_baseline() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE);
    memset(src, 0xAB, SIZE);

    for (size_t i = 0; i < WARMUP; ++i) __builtin_memcpy(dst, src, SIZE);

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        __builtin_memcpy(dst, src, SIZE);
    }
    auto end = high_resolution_clock::now();

    // Prevent optimizer from eliminating work
    volatile uint8_t sink = dst[0];
    (void)sink;

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

double test_bincode_memcpy() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);
    memset(src, 0xAB, SIZE);

    for (size_t i = 0; i < WARMUP; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        __builtin_memcpy(dst + 8, src, SIZE);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        __builtin_memcpy(dst + 8, src, SIZE);
    }
    auto end = high_resolution_clock::now();

    volatile uint8_t sink = dst[0];
    (void)sink;

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

double test_header_after() {
    uint8_t* src = (uint8_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);
    memset(src, 0xAB, SIZE);

    for (size_t i = 0; i < WARMUP; ++i) {
        __builtin_memcpy(dst + 8, src, SIZE);
        *reinterpret_cast<uint64_t*>(dst) = 16384;
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        __builtin_memcpy(dst + 8, src, SIZE);
        *reinterpret_cast<uint64_t*>(dst) = 16384;
    }
    auto end = high_resolution_clock::now();

    volatile uint8_t sink = dst[0];
    (void)sink;

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

double test_avx512_8x_unrolled() {
    uint64_t* src = (uint64_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);

    for (size_t i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (size_t i = 0; i < WARMUP; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 8) {
            _mm512_storeu_si512(d+j,   _mm512_loadu_si512(s+j));
            _mm512_storeu_si512(d+j+1, _mm512_loadu_si512(s+j+1));
            _mm512_storeu_si512(d+j+2, _mm512_loadu_si512(s+j+2));
            _mm512_storeu_si512(d+j+3, _mm512_loadu_si512(s+j+3));
            _mm512_storeu_si512(d+j+4, _mm512_loadu_si512(s+j+4));
            _mm512_storeu_si512(d+j+5, _mm512_loadu_si512(s+j+5));
            _mm512_storeu_si512(d+j+6, _mm512_loadu_si512(s+j+6));
            _mm512_storeu_si512(d+j+7, _mm512_loadu_si512(s+j+7));
        }
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 8) {
            _mm512_storeu_si512(d+j,   _mm512_loadu_si512(s+j));
            _mm512_storeu_si512(d+j+1, _mm512_loadu_si512(s+j+1));
            _mm512_storeu_si512(d+j+2, _mm512_loadu_si512(s+j+2));
            _mm512_storeu_si512(d+j+3, _mm512_loadu_si512(s+j+3));
            _mm512_storeu_si512(d+j+4, _mm512_loadu_si512(s+j+4));
            _mm512_storeu_si512(d+j+5, _mm512_loadu_si512(s+j+5));
            _mm512_storeu_si512(d+j+6, _mm512_loadu_si512(s+j+6));
            _mm512_storeu_si512(d+j+7, _mm512_loadu_si512(s+j+7));
        }
    }
    auto end = high_resolution_clock::now();

    volatile uint8_t sink = dst[0];
    (void)sink;

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  BEYOND LIMITS - Real benchmarks (1000 iterations)\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    double baseline = test_baseline();
    double bincode = test_bincode_memcpy();
    double header_after = test_header_after();
    double avx512 = test_avx512_8x_unrolled();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "1. Pure memcpy (aligned):          " << std::setw(7) << baseline << " GB/s  [100.0%]\n";
    std::cout << "2. Bincode (header + memcpy+8):    " << std::setw(7) << bincode << " GB/s  ["
              << (bincode/baseline*100) << "%]\n";
    std::cout << "3. Header AFTER data:              " << std::setw(7) << header_after << " GB/s  ["
              << (header_after/baseline*100) << "%]\n";
    std::cout << "4. AVX-512 8x unrolled:            " << std::setw(7) << avx512 << " GB/s  ["
              << (avx512/baseline*100) << "%]\n";

    double best = std::max({bincode, header_after, avx512});
    
    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "  BEST BINCODE: " << best << " GB/s (" << (best/baseline*100) << "% efficiency)\n";
    std::cout << "  Gap from baseline: " << (baseline - best) << " GB/s\n";

    if (best/baseline >= 0.99) {
        std::cout << "  ✓ SUCCESS: Reached 99%+ efficiency!\n";
    } else {
        std::cout << "  Need " << ((0.99 * baseline) - best) << " GB/s more for 99%\n";
    }
    std::cout << "═══════════════════════════════════════════════════════════\n";

    return 0;
}

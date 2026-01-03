// FINAL ASSAULT - 32x unrolling + prefetch + optimized scheduling
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

double test_avx512_32x_unrolled_prefetch() {
    uint64_t* src = (uint64_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);

    for (size_t i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 100; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 32) {
            // Prefetch ahead
            __builtin_prefetch(s + j + 32, 0, 3);
            __builtin_prefetch(s + j + 40, 0, 3);
            
            // 32x unrolled - loads first
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
            __m512i v16 = _mm512_loadu_si512(s+j+16);
            __m512i v17 = _mm512_loadu_si512(s+j+17);
            __m512i v18 = _mm512_loadu_si512(s+j+18);
            __m512i v19 = _mm512_loadu_si512(s+j+19);
            __m512i v20 = _mm512_loadu_si512(s+j+20);
            __m512i v21 = _mm512_loadu_si512(s+j+21);
            __m512i v22 = _mm512_loadu_si512(s+j+22);
            __m512i v23 = _mm512_loadu_si512(s+j+23);
            __m512i v24 = _mm512_loadu_si512(s+j+24);
            __m512i v25 = _mm512_loadu_si512(s+j+25);
            __m512i v26 = _mm512_loadu_si512(s+j+26);
            __m512i v27 = _mm512_loadu_si512(s+j+27);
            __m512i v28 = _mm512_loadu_si512(s+j+28);
            __m512i v29 = _mm512_loadu_si512(s+j+29);
            __m512i v30 = _mm512_loadu_si512(s+j+30);
            __m512i v31 = _mm512_loadu_si512(s+j+31);

            // Then stores
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
            _mm512_storeu_si512(d+j+16, v16);
            _mm512_storeu_si512(d+j+17, v17);
            _mm512_storeu_si512(d+j+18, v18);
            _mm512_storeu_si512(d+j+19, v19);
            _mm512_storeu_si512(d+j+20, v20);
            _mm512_storeu_si512(d+j+21, v21);
            _mm512_storeu_si512(d+j+22, v22);
            _mm512_storeu_si512(d+j+23, v23);
            _mm512_storeu_si512(d+j+24, v24);
            _mm512_storeu_si512(d+j+25, v25);
            _mm512_storeu_si512(d+j+26, v26);
            _mm512_storeu_si512(d+j+27, v27);
            _mm512_storeu_si512(d+j+28, v28);
            _mm512_storeu_si512(d+j+29, v29);
            _mm512_storeu_si512(d+j+30, v30);
            _mm512_storeu_si512(d+j+31, v31);
        }
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 32) {
            __builtin_prefetch(s + j + 32, 0, 3);
            __builtin_prefetch(s + j + 40, 0, 3);
            
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
            __m512i v16 = _mm512_loadu_si512(s+j+16);
            __m512i v17 = _mm512_loadu_si512(s+j+17);
            __m512i v18 = _mm512_loadu_si512(s+j+18);
            __m512i v19 = _mm512_loadu_si512(s+j+19);
            __m512i v20 = _mm512_loadu_si512(s+j+20);
            __m512i v21 = _mm512_loadu_si512(s+j+21);
            __m512i v22 = _mm512_loadu_si512(s+j+22);
            __m512i v23 = _mm512_loadu_si512(s+j+23);
            __m512i v24 = _mm512_loadu_si512(s+j+24);
            __m512i v25 = _mm512_loadu_si512(s+j+25);
            __m512i v26 = _mm512_loadu_si512(s+j+26);
            __m512i v27 = _mm512_loadu_si512(s+j+27);
            __m512i v28 = _mm512_loadu_si512(s+j+28);
            __m512i v29 = _mm512_loadu_si512(s+j+29);
            __m512i v30 = _mm512_loadu_si512(s+j+30);
            __m512i v31 = _mm512_loadu_si512(s+j+31);

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
            _mm512_storeu_si512(d+j+16, v16);
            _mm512_storeu_si512(d+j+17, v17);
            _mm512_storeu_si512(d+j+18, v18);
            _mm512_storeu_si512(d+j+19, v19);
            _mm512_storeu_si512(d+j+20, v20);
            _mm512_storeu_si512(d+j+21, v21);
            _mm512_storeu_si512(d+j+22, v22);
            _mm512_storeu_si512(d+j+23, v23);
            _mm512_storeu_si512(d+j+24, v24);
            _mm512_storeu_si512(d+j+25, v25);
            _mm512_storeu_si512(d+j+26, v26);
            _mm512_storeu_si512(d+j+27, v27);
            _mm512_storeu_si512(d+j+28, v28);
            _mm512_storeu_si512(d+j+29, v29);
            _mm512_storeu_si512(d+j+30, v30);
            _mm512_storeu_si512(d+j+31, v31);
        }
    }
    auto end = high_resolution_clock::now();

    volatile uint8_t sink = dst[SIZE-1];
    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  FINAL ASSAULT - 32x unrolling + prefetch\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    double baseline = test_baseline();
    double avx512_32x = test_avx512_32x_unrolled_prefetch();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Baseline (pure memcpy):    " << baseline << " GB/s  [100.00%]\n";
    std::cout << "AVX-512 32x + prefetch:    " << avx512_32x << " GB/s  ["
              << (avx512_32x/baseline*100) << "%]\n\n";

    if (avx512_32x/baseline >= 0.99) {
        std::cout << "✓✓✓ SUCCESS: REACHED 99%+ EFFICIENCY! ✓✓✓\n";
        std::cout << "Achieved " << (avx512_32x/baseline*100) << "% efficiency!\n";
    } else {
        std::cout << "Current: " << (avx512_32x/baseline*100) << "% efficiency\n";
        std::cout << "Gap: " << (baseline - avx512_32x) << " GB/s\n";
        std::cout << "Need " << ((0.99 * baseline) - avx512_32x) << " GB/s more for 99%\n";
    }

    std::cout << "═══════════════════════════════════════════════════════════\n";
    return 0;
}

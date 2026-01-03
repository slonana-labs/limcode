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
    for (size_t i = 0; i < ITERS; ++i) __builtin_memcpy(dst, src, SIZE);
    auto end = high_resolution_clock::now();

    volatile uint8_t sink = dst[SIZE-1];
    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

double test_non_temporal() {
    uint64_t* src = (uint64_t*)aligned_alloc(64, SIZE);
    uint8_t* dst = (uint8_t*)aligned_alloc(64, SIZE + 64);

    for (size_t i = 0; i < SIZE/8; ++i) src[i] = 0xABABABABABABABABULL;

    for (int i = 0; i < 100; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 4) {
            _mm512_stream_si512(d+j, _mm512_loadu_si512(s+j));
            _mm512_stream_si512(d+j+1, _mm512_loadu_si512(s+j+1));
            _mm512_stream_si512(d+j+2, _mm512_loadu_si512(s+j+2));
            _mm512_stream_si512(d+j+3, _mm512_loadu_si512(s+j+3));
        }
        _mm_sfence();
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        *reinterpret_cast<uint64_t*>(dst) = 16384;
        const __m512i* s = (const __m512i*)src;
        __m512i* d = (__m512i*)(dst + 8);

        for (size_t j = 0; j < SIZE/64; j += 4) {
            _mm512_stream_si512(d+j, _mm512_loadu_si512(s+j));
            _mm512_stream_si512(d+j+1, _mm512_loadu_si512(s+j+1));
            _mm512_stream_si512(d+j+2, _mm512_loadu_si512(s+j+2));
            _mm512_stream_si512(d+j+3, _mm512_loadu_si512(s+j+3));
        }
        _mm_sfence();
    }
    auto end = high_resolution_clock::now();

    volatile uint8_t sink = dst[SIZE-1];
    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    free(src); free(dst);
    return SIZE / ns;
}

int main() {
    std::cout << "Non-temporal stores (bypass cache):\n\n";
    
    double baseline = test_baseline();
    double non_temp = test_non_temporal();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Baseline:          " << baseline << " GB/s  [100.00%]\n";
    std::cout << "Non-temporal:      " << non_temp << " GB/s  [" << (non_temp/baseline*100) << "%]\n\n";

    if (non_temp/baseline >= 0.99) {
        std::cout << "✓✓✓ 99%+ ACHIEVED! ✓✓✓\n";
    } else {
        std::cout << "Gap: " << ((0.99 * baseline) - non_temp) << " GB/s needed for 99%\n";
    }

    return 0;
}

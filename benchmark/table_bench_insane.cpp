/**
 * @file table_bench_insane.cpp
 * @brief INSANE MODE - 64-byte alignment + __restrict__ + manual optimization
 */

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>

using namespace std::chrono;

double benchmark_insane(size_t num_elements, size_t iterations) {
    const size_t data_size = num_elements * sizeof(uint64_t);

    // 64-byte aligned allocation for AVX-512
    alignas(64) static uint64_t data_storage[262144];  // Max 2MB
    for (size_t i = 0; i < num_elements; ++i) {
        data_storage[i] = 0xABCDEF;
    }

    // Pre-allocate buffer with alignment
    std::vector<uint8_t> buf;
    buf.reserve(data_size + 8);

    // Warmup
    for (size_t i = 0; i < 3; ++i) {
        buf.resize(data_size + 8);
        uint8_t* __restrict__ ptr = buf.data();
        const uint8_t* __restrict__ src = (const uint8_t*)data_storage;
        *reinterpret_cast<uint64_t*>(ptr) = num_elements;
        __builtin_memcpy(ptr + 8, src, data_size);
    }

    // Benchmark with __restrict__ pointers
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        buf.resize(data_size + 8);

        uint8_t* __restrict__ ptr = buf.data();
        const uint8_t* __restrict__ src = (const uint8_t*)data_storage;

        *reinterpret_cast<uint64_t*>(ptr) = num_elements;
        __builtin_memcpy(ptr + 8, src, data_size);
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);
    return data_size / ns_per_op;
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
    std::cout << "INSANE MODE Benchmark (aligned + restrict)\n\n";
    std::cout << "Size,Throughput_GBps\n";

    struct SizeConfig {
        size_t num_elements;
        size_t iterations;
    };

    std::vector<SizeConfig> sizes = {
        {8, 5000},           // 64B
        {16, 5000},          // 128B
        {32, 5000},          // 256B
        {64, 2500},          // 512B
        {128, 1000},         // 1KB
        {256, 500},          // 2KB
        {512, 250},          // 4KB
        {1024, 100},         // 8KB
        {2048, 50},          // 16KB
        {4096, 25},          // 32KB
        {8192, 10},          // 64KB
        {16384, 5},          // 128KB
        {32768, 3},          // 256KB
        {65536, 2},          // 512KB
        {131072, 2},         // 1MB
        {262144, 1},         // 2MB
    };

    for (const auto& cfg : sizes) {
        size_t size_bytes = cfg.num_elements * sizeof(uint64_t);
        double throughput = benchmark_insane(cfg.num_elements, cfg.iterations);

        std::cout << format_size(size_bytes) << ","
                  << std::fixed << std::setprecision(2) << throughput << "\n";
        std::cout.flush();
    }

    std::cout << "\nINSANE MODE complete.\n";
    return 0;
}

/**
 * @file bench.cpp
 * @brief Performance benchmark suite for limcode serialization
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace std::chrono;

double benchmark_size(size_t num_elements, size_t iterations) {
    const size_t data_bytes = num_elements * sizeof(uint64_t);

    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = 0xABCDEF0123456789ULL;  // Realistic pattern
    }

    std::vector<uint8_t> buf;

    // Warmup
    for (size_t i = 0; i < 3; ++i) {
        limcode::serialize(buf, data);
    }

    // Benchmark
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        limcode::serialize(buf, data);
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);

    // Prevent optimization - use the buffer
    volatile uint8_t sink = buf[0];
    (void)sink;

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
    std::cout << "Limcode Serialization Benchmark\n\n";
    std::cout << "Size,Throughput_GBps\n";

    struct SizeConfig {
        size_t num_elements;
        size_t iterations;
    };

    std::vector<SizeConfig> sizes = {
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
        double gbps = benchmark_size(cfg.num_elements, cfg.iterations);
        std::cout << format_size(size_bytes) << "," << std::fixed << std::setprecision(2) << gbps << "\n";
    }

    return 0;
}

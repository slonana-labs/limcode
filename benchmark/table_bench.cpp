/**
 * @file table_bench.cpp
 * @brief C++ Limcode benchmark for README table generation
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>

using namespace std::chrono;

double benchmark_roundtrip(size_t num_elements, size_t iterations) {
    // Create test data
    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    const size_t data_size = num_elements * sizeof(uint64_t);
    std::vector<uint8_t> buf;

    // Warmup
    for (size_t i = 0; i < std::min(iterations / 10, size_t(3)); ++i) {
        limcode::serialize(buf, data);
    }

    // Benchmark
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        limcode::serialize(buf, data);
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);
    double throughput_gbps = (data_size / ns_per_op); // GiB/s

    return throughput_gbps;
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
    std::cout << "C++ Limcode Benchmark\n\n";

    // Match Rust benchmark sizes exactly
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
        {524288, 1},         // 4MB
        {1048576, 1},        // 8MB
        {2097152, 1},        // 16MB
        {4194304, 1},        // 32MB
        {8388608, 1},        // 64MB
        {16777216, 1},       // 128MB
    };

    std::cout << "Size,Throughput_GBps\n";

    for (const auto& cfg : sizes) {
        size_t size_bytes = cfg.num_elements * sizeof(uint64_t);
        double throughput = benchmark_roundtrip(cfg.num_elements, cfg.iterations);

        // Output CSV format: Size,Throughput
        std::cout << format_size(size_bytes) << ","
                  << std::fixed << std::setprecision(2) << throughput << "\n";
        std::cout.flush();
    }

    std::cout << "\nBenchmark complete.\n";
    return 0;
}

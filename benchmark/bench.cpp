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

void benchmark_size(const char* label, size_t num_elements) {
    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    const size_t data_bytes = num_elements * sizeof(uint64_t);
    const size_t iterations = std::max(size_t(10), 100'000'000 / (data_bytes + 1));

    std::vector<uint8_t> buf;
    
    // Warmup
    for (size_t i = 0; i < 10; ++i) {
        limcode::serialize(buf, data);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        limcode::serialize(buf, data);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count();
    double ns_per_op = ns / iterations;
    double gbps = (data_bytes / ns_per_op);

    std::cout << std::left << std::setw(15) << label
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) 
              << (data_bytes / 1024.0) << " KB,"
              << std::setw(10) << gbps << " GB/s\n";
}

int main() {
    std::cout << "Limcode Serialization Benchmark\n";
    std::cout << "================================\n\n";
    std::cout << std::left << std::setw(15) << "Size" 
              << std::right << std::setw(20) << "Throughput\n";
    std::cout << std::string(45, '-') << "\n";

    benchmark_size("1KB", 128);
    benchmark_size("4KB", 512);
    benchmark_size("16KB", 2048);
    benchmark_size("64KB", 8192);
    benchmark_size("128KB", 16384);
    benchmark_size("256KB", 32768);
    benchmark_size("1MB", 131072);
    benchmark_size("4MB", 524288);
    benchmark_size("16MB", 2097152);

    std::cout << "\n";
    return 0;
}

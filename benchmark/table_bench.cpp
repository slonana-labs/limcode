/**
 * @file table_bench.cpp
 * @brief Table-format benchmark matching Rust benchmark sizes
 * Outputs CSV for README table generation
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <cstring>

using namespace std::chrono;
using namespace limcode;

// Simple POD serialization (bincode compatible)
template<typename T>
std::vector<uint8_t> serialize_vec(const std::vector<T>& data) {
    static_assert(std::is_trivially_copyable_v<T>, "Must be POD type");

    const size_t count = data.size();
    const size_t data_bytes = count * sizeof(T);
    const size_t total_size = 8 + data_bytes; // 8 bytes for length prefix

    std::vector<uint8_t> buffer(total_size);

    // Write length prefix (little-endian uint64_t)
    std::memcpy(buffer.data(), &count, 8);

    // Write data
    std::memcpy(buffer.data() + 8, data.data(), data_bytes);

    return buffer;
}

double benchmark_roundtrip(size_t num_elements, size_t iterations) {
    // Create test data
    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    const size_t data_size = num_elements * sizeof(uint64_t);

    // Warmup
    for (size_t i = 0; i < std::min(iterations / 10, size_t(3)); ++i) {
        auto buf = serialize_vec(data);
    }

    // Benchmark
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        auto buf = serialize_vec(data);
        // Touch the buffer to ensure it's not optimized away
        volatile uint8_t x = buf[0];
        (void)x;
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
    std::cout << "C++ Limcode Table Benchmark\n";
    std::cout << "============================\n\n";

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

    std::cout << "\nBenchmark complete!\n";
    return 0;
}

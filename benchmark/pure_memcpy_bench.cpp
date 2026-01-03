/**
 * Pure memcpy benchmark - no serialization overhead
 */

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>
#include <cstring>

using namespace std::chrono;

double benchmark_pure_memcpy(size_t num_bytes, size_t iterations) {
    // Allocate source and destination buffers
    std::vector<uint8_t> src(num_bytes);
    std::vector<uint8_t> dst(num_bytes);

    // Initialize source with data
    for (size_t i = 0; i < num_bytes; ++i) {
        src[i] = i & 0xFF;
    }

    // Warm up
    for (size_t i = 0; i < 3; ++i) {
        __builtin_memcpy(dst.data(), src.data(), num_bytes);
    }

    // Benchmark
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        __builtin_memcpy(dst.data(), src.data(), num_bytes);
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);
    double throughput_gbps = (num_bytes / ns_per_op);

    return throughput_gbps;
}

int main() {
    std::cout << "Pure memcpy Benchmark (no serialization overhead)\n\n";
    std::cout << "Size,Throughput_GBps\n";

    struct TestCase {
        const char* name;
        size_t bytes;
        size_t iterations;
    };

    TestCase cases[] = {
        {"128KB", 131072, 1000},
        {"256KB", 262144, 500},
        {"512KB", 524288, 250},
        {"1MB", 1048576, 100},
    };

    for (const auto& tc : cases) {
        double throughput = benchmark_pure_memcpy(tc.bytes, tc.iterations);
        std::cout << tc.name << ","
                  << std::fixed << std::setprecision(2) << throughput << "\n";
        std::cout.flush();
    }

    std::cout << "\nBenchmark complete.\n";
    return 0;
}

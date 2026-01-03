/**
 * Direct serialize benchmark matching pure_memcpy structure
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>

using namespace std::chrono;

double benchmark_serialize_direct(size_t num_bytes, size_t iterations) {
    // Create source data (uint8_t like pure memcpy)
    const size_t num_elements = num_bytes / sizeof(uint64_t);
    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    std::vector<uint8_t> buf;

    // Warm up
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
    double throughput_gbps = (num_bytes / ns_per_op);

    return throughput_gbps;
}

int main() {
    std::cout << "Direct Serialize Benchmark\n\n";
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
        double throughput = benchmark_serialize_direct(tc.bytes, tc.iterations);
        std::cout << tc.name << ","
                  << std::fixed << std::setprecision(2) << throughput << "\n";
        std::cout.flush();
    }

    std::cout << "\nBenchmark complete.\n";
    return 0;
}

// Standalone serialize benchmark - EXACT copy of table_bench pattern
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace std::chrono;

// EXACT copy from table_bench.cpp
double benchmark_roundtrip(size_t num_elements, size_t iterations) {
    const size_t data_size = num_elements * sizeof(uint64_t);

    // Create test data - keep it simple
    std::vector<uint64_t> data(num_elements, 0xABCDEF);
    std::vector<uint8_t> buf;

    // Warmup
    for (size_t i = 0; i < 3; ++i) {
        const size_t count = data.size();
        const size_t bytes = count * sizeof(uint64_t);
        const size_t total = 8 + bytes;

        if (buf.capacity() < total) buf.reserve(total);
        buf.resize(total);

        uint8_t* ptr = buf.data();
        *reinterpret_cast<uint64_t*>(ptr) = count;
        __builtin_memcpy(ptr + 8, data.data(), bytes);
    }

    // Benchmark - inline everything
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        const size_t count = data.size();
        const size_t bytes = count * sizeof(uint64_t);
        const size_t total = 8 + bytes;

        if (buf.capacity() < total) buf.reserve(total);
        buf.resize(total);

        uint8_t* ptr = buf.data();
        *reinterpret_cast<uint64_t*>(ptr) = count;
        __builtin_memcpy(ptr + 8, data.data(), bytes);
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);
    return data_size / ns_per_op;
}

int main() {
    std::cout << std::fixed << std::setprecision(2);

    // Run multiple times like table_bench does (it runs 11 tests before 128KB)
    std::cout << "Running warmup tests...\n";
    for (int i = 0; i < 10; ++i) {
        benchmark_roundtrip(16384, 5);
    }

    // Now test with 128KB exactly like table_bench
    std::cout << "Running actual test...\n";
    double throughput = benchmark_roundtrip(16384, 5);  // 16384 elements, 5 iterations

    std::cout << "Standalone (table_bench pattern): " << throughput << " GB/s\n";
    std::cout << "(Should match table_bench: ~69-70 GB/s)\n";

    return 0;
}

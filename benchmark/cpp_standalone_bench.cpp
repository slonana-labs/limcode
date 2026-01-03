/**
 * @file cpp_standalone_bench.cpp
 * @brief C++ standalone performance benchmark (no Rust dependency)
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <iomanip>

using namespace limcode;
using namespace std::chrono;

template<typename Func>
double benchmark(const char* name, Func&& func, size_t iterations, size_t data_size) {
    // Warmup
    for (size_t i = 0; i < std::min(iterations / 10, size_t(1000)); ++i) {
        func();
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        func();
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);
    double throughput_gbps = (data_size / ns_per_op) * 1.0;

    std::cout << std::left << std::setw(25) << name
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << ns_per_op << " ns/op  "
              << std::setw(10) << std::setprecision(2) << throughput_gbps << " GiB/s\n";

    return ns_per_op;
}

void bench_vec_u64(size_t num_elements) {
    std::cout << "\n[Vec<u64> with " << num_elements << " elements (" << (num_elements * 8 / 1024) << " KB)]\n";

    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    size_t data_size = num_elements * sizeof(uint64_t);
    size_t iterations = std::max(size_t(10), 100000000 / (data_size + 1));

    benchmark("serialize", [&]() {
        LimcodeEncoder enc;
        enc.write_u64(num_elements);
        enc.write_bytes(reinterpret_cast<const uint8_t*>(data.data()), data_size);
        volatile auto bytes = std::move(enc).finish();
    }, iterations, data_size);
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  C++ Limcode Standalone Performance Benchmark\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";

    bench_vec_u64(8);       // 64B
    bench_vec_u64(128);     // 1KB
    bench_vec_u64(512);     // 4KB
    bench_vec_u64(2048);    // 16KB
    bench_vec_u64(8192);    // 64KB
    bench_vec_u64(32768);   // 256KB
    bench_vec_u64(131072);  // 1MB
    bench_vec_u64(8388608); // 64MB

    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "Target: 64MB should be >10 GiB/s (matching Rust)\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    return 0;
}

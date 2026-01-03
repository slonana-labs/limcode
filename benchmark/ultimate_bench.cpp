/**
 * @file ultimate_bench.cpp
 * @brief ULTIMATE mode - Match hardware maximum (22.39 GiB/s)
 */

#include <limcode/ultimate_fast.h>
#include <limcode/insane_fast.h>
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace std::chrono;

void print_separator() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
}

template<typename Func>
double benchmark(const char* name, Func&& func, size_t iterations, size_t data_size) {
    // Warmup
    for (size_t i = 0; i < std::min(iterations / 10, size_t(10)); ++i) {
        func();
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        func();
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);
    double throughput_gbps = (data_size / ns_per_op);

    std::cout << std::left << std::setw(45) << name
              << std::right << std::setw(12) << std::fixed << std::setprecision(2)
              << ns_per_op << " ns  "
              << std::setw(10) << std::setprecision(2) << throughput_gbps << " GiB/s";

    // Show vs hardware max
    double hw_max = 22.39;
    double percent = (throughput_gbps / hw_max) * 100;

    if (percent >= 99.0) {
        std::cout << "  🎯 " << std::fixed << std::setprecision(1) << percent << "%";
    } else if (percent >= 95.0) {
        std::cout << "  ⚡ " << std::fixed << std::setprecision(1) << percent << "%";
    } else {
        std::cout << "     " << std::fixed << std::setprecision(1) << percent << "%";
    }

    std::cout << "\n";
    return throughput_gbps;
}

void bench_comparison(size_t num_elements) {
    size_t size_mb = num_elements * 8 / (1024 * 1024);
    std::cout << "\n[" << num_elements << " elements (" << size_mb << " MB)]\n";

    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    size_t data_size = num_elements * sizeof(uint64_t);
    size_t iterations = std::max(size_t(10), 50000000 / (data_size + 1));

    // INSANE mode (16x unrolling - 1024 bytes/iter)
    std::vector<uint8_t> buf_insane;
    benchmark("INSANE (16x unrolling, 1024 bytes/iter)", [&]() {
        limcode::insane_fast::serialize_pod_into_insane(buf_insane, data);
    }, iterations, data_size);

    // ULTIMATE mode (32x unrolling - 2048 bytes/iter)
    std::vector<uint8_t> buf_ultimate;
    benchmark("ULTIMATE (32x unrolling, 2048 bytes/iter)", [&]() {
        limcode::ultimate_fast::serialize_pod_into_ultimate(buf_ultimate, data);
    }, iterations, data_size);
}

void bench_raw_memcpy() {
    std::cout << "\n[Raw Memory Bandwidth - ULTIMATE Mode]\n";

    const size_t SIZE = 64 * 1024 * 1024; // 64MB (smaller to test)
    std::vector<uint8_t> src(SIZE, 0x42);
    std::vector<uint8_t> dst(SIZE, 0x00);

    // Test single-threaded first
    auto start1 = high_resolution_clock::now();
    limcode::ultimate_fast::ultimate_memcpy(dst.data(), src.data(), SIZE);
    auto end1 = high_resolution_clock::now();

    double ns1 = duration_cast<nanoseconds>(end1 - start1).count();
    double gbps1 = (SIZE / ns1);
    double percent1 = (gbps1 / 22.39) * 100;

    std::cout << "64MB single-threaded (32x unrolling): " << std::fixed << std::setprecision(2)
              << gbps1 << " GiB/s (" << std::setprecision(1) << percent1 << "% of HW max)\n";
}

int main() {
    print_separator();
    std::cout << "  ULTIMATE C++ Limcode Benchmark\n";
    print_separator();
    std::cout << "\nOptimizations:\n";
    std::cout << "  - 32x SIMD unrolling (2048 bytes/iteration)\n";
    std::cout << "  - Aggressive prefetching (4KB ahead)\n";
    std::cout << "  - Multi-threaded parallel copy\n";
    std::cout << "  - Zero allocation overhead\n";
    std::cout << "  - Compiler pragma unroll\n";
    std::cout << "\nHardware Maximum: 22.39 GiB/s\n";
    std::cout << "Target: 100% of hardware max\n";

    // Raw memcpy test
    bench_raw_memcpy();

    // Comparison tests
    bench_comparison(1048576);    // 8MB
    bench_comparison(4194304);    // 32MB
    bench_comparison(8388608);    // 64MB - CRITICAL
    bench_comparison(16777216);   // 128MB
    bench_comparison(33554432);   // 256MB

    print_separator();
    std::cout << "ULTIMATE mode complete!\n";
    std::cout << "Target: Match hardware maximum (22.39 GiB/s)\n";
    print_separator();

    return 0;
}

/**
 * @file insane_bench.cpp
 * @brief INSANE mode benchmark - Hand-tuned assembly, 16x unrolling, 32 ZMM registers
 * Target: 20+ GiB/s (90% of hardware max)
 */

#include <limcode/extreme_fast.h>
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

    std::cout << std::left << std::setw(40) << name
              << std::right << std::setw(12) << std::fixed << std::setprecision(2)
              << ns_per_op << " ns/op  "
              << std::setw(10) << std::setprecision(2) << throughput_gbps << " GiB/s";

    // Show vs hardware max
    if (data_size >= 1024 * 1024) {
        double hw_max = 22.39;  // Measured hardware max
        double percent = (throughput_gbps / hw_max) * 100;
        std::cout << "  (" << std::fixed << std::setprecision(1) << percent << "% of HW max)";
    }

    std::cout << "\n";
    return throughput_gbps;
}

void bench_comparison(size_t num_elements) {
    size_t size_kb = num_elements * 8 / 1024;
    std::cout << "\n[" << num_elements << " elements (" << size_kb << " KB)]\n";

    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    size_t data_size = num_elements * sizeof(uint64_t);
    size_t iterations = std::max(size_t(10), 100000000 / (data_size + 1));

    // EXTREME mode (8x unrolling)
    std::vector<uint8_t> buf_extreme;
    benchmark("EXTREME (8x unrolling)", [&]() {
        limcode::extreme_fast::serialize_pod_into_extreme(buf_extreme, data);
    }, iterations, data_size);

    // INSANE mode (16x unrolling + inline asm)
    std::vector<uint8_t> buf_insane;
    benchmark("INSANE (16x unrolling + asm)", [&]() {
        limcode::insane_fast::serialize_pod_into_insane(buf_insane, data);
    }, iterations, data_size);
}

void bench_raw_memcpy() {
    std::cout << "\n[Raw Memory Bandwidth - INSANE Mode]\n";

    const size_t SIZE = 128 * 1024 * 1024; // 128MB
    std::vector<uint8_t> src(SIZE, 0x42);
    std::vector<uint8_t> dst(SIZE, 0x00);

    auto start = high_resolution_clock::now();
    limcode::insane_fast::insane_memcpy_parallel(dst.data(), src.data(), SIZE);
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count();
    double gbps = (SIZE / ns);

    std::cout << "128MB parallel copy (inline asm): " << std::fixed << std::setprecision(2)
              << gbps << " GiB/s\n";
    std::cout << "This should be close to hardware maximum (~22 GiB/s).\n";
}

int main() {
    print_separator();
    std::cout << "  INSANE C++ Limcode Benchmark\n";
    print_separator();
    std::cout << "\nOptimizations:\n";
    std::cout << "  - Hand-written inline assembly\n";
    std::cout << "  - 16x SIMD unrolling (1024 bytes/iteration)\n";
    std::cout << "  - All 32 ZMM registers used\n";
    std::cout << "  - Aggressive prefetching (2KB ahead)\n";
    std::cout << "  - Zero allocation overhead\n";
    std::cout << "\nTarget: 20+ GiB/s (90% of hardware max)\n";

    // Raw memcpy test
    bench_raw_memcpy();

    // Comparison tests
    bench_comparison(131072);     // 1MB
    bench_comparison(1048576);    // 8MB
    bench_comparison(4194304);    // 32MB
    bench_comparison(8388608);    // 64MB - CRITICAL TEST
    bench_comparison(16777216);   // 128MB

    print_separator();
    std::cout << "INSANE mode complete!\n";
    std::cout << "Expected: 15-20 GiB/s on this hardware (70-90% of max).\n";
    print_separator();

    return 0;
}

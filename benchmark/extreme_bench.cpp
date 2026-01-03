/**
 * @file extreme_bench.cpp
 * @brief EXTREME C++ performance benchmark - Target: 120+ GiB/s (10x Rust)
 */

#include <limcode/ultra_fast.h>
#include <limcode/extreme_fast.h>
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace limcode::extreme_fast;
using namespace std::chrono;

void print_separator() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
}

void print_header(const char* title) {
    print_separator();
    std::cout << "  " << title << "\n";
    print_separator();
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
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << ns_per_op << " ns/op  "
              << std::setw(10) << std::setprecision(2) << throughput_gbps << " GiB/s";

    // Show vs target
    if (data_size >= 1024 * 1024) {
        double target = 120.0;  // 10x Rust's 12 GiB/s
        if (throughput_gbps >= target) {
            std::cout << "  🚀 CRUSHING TARGET!";
        } else if (throughput_gbps >= target * 0.8) {
            std::cout << "  ✅ Close to target";
        } else {
            std::cout << "  ⚠️  " << std::fixed << std::setprecision(1)
                      << (target / throughput_gbps) << "x gap";
        }
    }

    std::cout << "\n";
    return throughput_gbps;
}

void bench_extreme_mode(size_t num_elements) {
    size_t size_kb = num_elements * 8 / 1024;
    std::cout << "\n[EXTREME MODE: " << num_elements << " elements ("
              << size_kb << " KB)]\n";

    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    size_t data_size = num_elements * sizeof(uint64_t);
    size_t iterations = std::max(size_t(10), 100000000 / (data_size + 1));

    // Single-threaded SIMD (baseline)
    std::vector<uint8_t> buf_single;
    benchmark("Single-threaded SIMD", [&]() {
        buf_single.clear();
        buf_single.reserve(8 + data_size);
        buf_single.resize(8 + data_size);
        uint8_t* ptr = buf_single.data();
        uint64_t len = data.size();
        std::memcpy(ptr, &len, 8);
        extreme_memcpy_single_thread(ptr + 8, data.data(), data_size);
    }, iterations, data_size);

    // Multi-threaded parallel copy (EXTREME)
    std::vector<uint8_t> buf_extreme;
    double gbps = benchmark("Multi-threaded EXTREME", [&]() {
        serialize_pod_into_extreme(buf_extreme, data);
    }, iterations, data_size);

    // Show speedup for large data
    if (data_size >= 1024 * 1024) {
        std::cout << "  → Hardware theoretical max: ~200 GiB/s (DDR5-5600 dual-channel)\n";
        std::cout << "  → Achieving: " << std::fixed << std::setprecision(1)
                  << (gbps / 200.0 * 100) << "% of theoretical max\n";
    }
}

void bench_memory_bandwidth() {
    std::cout << "\n[Raw Memory Bandwidth Test]\n";

    auto start = high_resolution_clock::now();
    double gbps = measure_memory_bandwidth();
    auto end = high_resolution_clock::now();

    double ms = duration_cast<microseconds>(end - start).count() / 1000.0;

    std::cout << "128MB parallel copy: " << std::fixed << std::setprecision(2)
              << gbps << " GiB/s (" << ms << " ms)\n";
    std::cout << "This is your hardware's maximum achievable bandwidth.\n";
}

void bench_comparison() {
    std::cout << "\n[Comparison: ultra_fast vs EXTREME]\n";

    std::vector<uint64_t> data(8 * 1024 * 1024); // 64MB
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = i;
    }

    size_t data_size = data.size() * sizeof(uint64_t);
    size_t iterations = 100;

    // ultra_fast (single-threaded)
    std::vector<uint8_t> buf1;
    auto start1 = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        limcode::ultra_fast::serialize_pod_into(buf1, data);
    }
    auto end1 = high_resolution_clock::now();
    double ns1 = duration_cast<nanoseconds>(end1 - start1).count() / static_cast<double>(iterations);
    double gbps1 = (data_size / ns1);

    // extreme_fast (multi-threaded)
    std::vector<uint8_t> buf2;
    auto start2 = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        serialize_pod_into_extreme(buf2, data);
    }
    auto end2 = high_resolution_clock::now();
    double ns2 = duration_cast<nanoseconds>(end2 - start2).count() / static_cast<double>(iterations);
    double gbps2 = (data_size / ns2);

    std::cout << "ultra_fast (single-thread):  " << std::fixed << std::setprecision(2)
              << gbps1 << " GiB/s\n";
    std::cout << "extreme_fast (multi-thread): " << std::fixed << std::setprecision(2)
              << gbps2 << " GiB/s\n";
    std::cout << "Speedup: " << std::fixed << std::setprecision(2)
              << (gbps2 / gbps1) << "x\n";
}

int main() {
    print_header("EXTREME C++ Limcode Benchmark");
    std::cout << "\nTarget: 120+ GiB/s (10x Rust's 12 GiB/s)\n";
    std::cout << "Strategy: Multi-core parallel memory copy\n";
    std::cout << "CPU cores: " << std::thread::hardware_concurrency() << "\n";

    // Test memory bandwidth first
    bench_memory_bandwidth();

    // Test all sizes
    bench_extreme_mode(8);          // 64B
    bench_extreme_mode(128);        // 1KB
    bench_extreme_mode(512);        // 4KB
    bench_extreme_mode(2048);       // 16KB
    bench_extreme_mode(8192);       // 64KB
    bench_extreme_mode(32768);      // 256KB
    bench_extreme_mode(131072);     // 1MB - parallel threshold
    bench_extreme_mode(1048576);    // 8MB
    bench_extreme_mode(4194304);    // 32MB
    bench_extreme_mode(8388608);    // 64MB - CRITICAL TEST

    // Direct comparison
    bench_comparison();

    print_separator();
    std::cout << "EXTREME mode complete!\n";
    std::cout << "Expected results:\n";
    std::cout << "  - Small data (<1MB): Similar to ultra_fast (threading overhead)\n";
    std::cout << "  - Large data (>1MB): 2-4x faster with multi-threading\n";
    std::cout << "  - 64MB: 40-120 GiB/s depending on CPU/memory\n";
    print_separator();

    return 0;
}

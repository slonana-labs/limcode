/**
 * @file ultra_fast_bench.cpp
 * @brief Benchmark for ultra_fast.h zero-copy buffer reuse and parallel encoding
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <iomanip>

using namespace limcode::ultra_fast;
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
    for (size_t i = 0; i < std::min(iterations / 10, size_t(100)); ++i) {
        func();
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        func();
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);
    double throughput_gbps = (data_size / ns_per_op);

    std::cout << std::left << std::setw(35) << name
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << ns_per_op << " ns/op  "
              << std::setw(10) << std::setprecision(2) << throughput_gbps << " GiB/s\n";

    return throughput_gbps;
}

void bench_buffer_reuse(size_t num_elements) {
    std::cout << "\n[Buffer Reuse: " << num_elements << " elements ("
              << (num_elements * 8 / 1024) << " KB)]\n";

    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    size_t data_size = num_elements * sizeof(uint64_t);
    size_t iterations = std::max(size_t(10), 100000000 / (data_size + 1));

    // Allocating version (baseline)
    benchmark("serialize_pod (with alloc)", [&]() {
        volatile auto bytes = serialize_pod(data);
    }, iterations, data_size);

    // Zero-copy buffer reuse (target: 12+ GiB/s)
    std::vector<uint8_t> buf;
    double gbps = benchmark("serialize_pod_into (reuse)", [&]() {
        serialize_pod_into(buf, data);
    }, iterations, data_size);

    // Show improvement for large blocks
    if (data_size >= 1024 * 1024) {
        std::cout << "    → Target: 12+ GiB/s for large blocks\n";
        if (gbps >= 12.0) {
            std::cout << "    ✅ ACHIEVED TARGET!\n";
        } else {
            std::cout << "    ⚠️  Below target (" << std::fixed << std::setprecision(1)
                      << (12.0 / gbps) << "x gap)\n";
        }
    }
}

void bench_parallel_batch(size_t batch_size, size_t elements_per_vec) {
    std::cout << "\n[Parallel Batch: " << batch_size << " vectors × "
              << elements_per_vec << " elements]\n";

    // Generate batch input
    std::vector<std::vector<uint64_t>> inputs(batch_size);
    for (size_t i = 0; i < batch_size; ++i) {
        inputs[i].resize(elements_per_vec);
        for (size_t j = 0; j < elements_per_vec; ++j) {
            inputs[i][j] = i * elements_per_vec + j;
        }
    }

    size_t total_data_size = batch_size * elements_per_vec * sizeof(uint64_t);
    size_t iterations = std::max(size_t(10), 10000000 / (total_data_size + 1));

    // Sequential encoding (baseline)
    benchmark("Sequential encoding", [&]() {
        std::vector<std::vector<uint8_t>> outputs(batch_size);
        for (size_t i = 0; i < batch_size; ++i) {
            outputs[i] = serialize_pod(inputs[i]);
        }
        volatile auto result = outputs.size();
    }, iterations, total_data_size);

    // Parallel encoding
    benchmark("Parallel encoding", [&]() {
        volatile auto outputs = parallel_encode_batch(inputs);
    }, iterations, total_data_size);
}

void bench_throughput_api() {
    std::cout << "\n[Throughput API Test]\n";

    std::vector<uint64_t> data(1024); // 8KB
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = i;
    }

    double gbps = benchmark_throughput(data, 10000);
    std::cout << "Throughput (8KB, 10K iterations): " << std::fixed << std::setprecision(2)
              << gbps << " GiB/s\n";
}

void bench_memory_prefaulting() {
    std::cout << "\n[Memory Prefaulting Test (>16MB)]\n";

    // 32MB test (should trigger prefaulting)
    std::vector<uint64_t> large_data(4 * 1024 * 1024); // 32MB
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = i;
    }

    std::vector<uint8_t> buf;
    size_t iterations = 100;

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        serialize_pod_into(buf, large_data);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count();
    double bytes_per_iter = large_data.size() * sizeof(uint64_t);
    double gbps = (bytes_per_iter * iterations / ns);

    std::cout << "32MB with prefaulting: " << std::fixed << std::setprecision(2)
              << gbps << " GiB/s (" << iterations << " iterations)\n";
}

int main() {
    print_header("Ultra-Fast C++ Limcode Benchmark");
    std::cout << "\nTarget: Match Rust's 12 GiB/s buffer reuse performance\n";

    // Test buffer reuse across all sizes
    bench_buffer_reuse(8);         // 64B
    bench_buffer_reuse(128);       // 1KB
    bench_buffer_reuse(512);       // 4KB
    bench_buffer_reuse(2048);      // 16KB
    bench_buffer_reuse(8192);      // 64KB
    bench_buffer_reuse(32768);     // 256KB
    bench_buffer_reuse(131072);    // 1MB
    bench_buffer_reuse(8388608);   // 64MB - CRITICAL TEST

    // Test parallel batch encoding
    bench_parallel_batch(100, 1024);   // 100 × 8KB vectors
    bench_parallel_batch(1000, 128);   // 1000 × 1KB vectors

    // Test throughput API
    bench_throughput_api();

    // Test memory prefaulting
    bench_memory_prefaulting();

    print_separator();
    std::cout << "Benchmark complete!\n";
    std::cout << "Key metrics:\n";
    std::cout << "  - 64MB buffer reuse should be 12+ GiB/s\n";
    std::cout << "  - Parallel encoding should scale with CPU cores\n";
    print_separator();

    return 0;
}

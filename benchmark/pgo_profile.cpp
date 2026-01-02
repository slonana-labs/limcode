/**
 * @file pgo_profile.cpp
 * @brief Benchmark for Profile-Guided Optimization (PGO) training
 *
 * Usage:
 *   1. cmake .. -DENABLE_PGO_GENERATE=ON && make pgo_profile
 *   2. ./pgo_profile  (generates *.gcda profile data)
 *   3. cmake .. -DENABLE_PGO_USE=ON && make  (rebuilds with PGO)
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace limcode;
using namespace std::chrono;

// Generate realistic blockchain workload
std::vector<uint8_t> generate_random_data(size_t size) {
    static std::mt19937_64 rng(0x1337);
    std::vector<uint8_t> data(size);
    for (auto& byte : data) {
        byte = static_cast<uint8_t>(rng());
    }
    return data;
}

void benchmark_serialize(size_t size, size_t iterations) {
    auto data = generate_random_data(size);

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        LimcodeEncoder enc;
        enc.write_bytes(data.data(), data.size());
        volatile auto bytes = std::move(enc).finish();  // Prevent optimization
    }
    auto end = high_resolution_clock::now();

    auto ns_per_op = duration_cast<nanoseconds>(end - start).count() / iterations;
    std::cout << "Serialize " << size << "B: " << ns_per_op << "ns/op\n";
}

void benchmark_deserialize(size_t size, size_t iterations) {
    auto data = generate_random_data(size);

    LimcodeEncoder enc;
    enc.write_bytes(data.data(), data.size());
    auto encoded = std::move(enc).finish();

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        LimcodeDecoder dec(encoded.data(), encoded.size());
        std::vector<uint8_t> buffer(size);
        dec.read_bytes(buffer.data(), size);
        volatile auto ptr = buffer.data();  // Prevent optimization
    }
    auto end = high_resolution_clock::now();

    auto ns_per_op = duration_cast<nanoseconds>(end - start).count() / iterations;
    std::cout << "Deserialize " << size << "B: " << ns_per_op << "ns/op\n";
}

int main() {
    std::cout << "=== PGO Profile Generation Benchmark ===\n\n";

    // Cover common sizes for PGO profiling
    const std::vector<std::pair<size_t, size_t>> sizes = {
        {64, 10'000'000},    // Signatures
        {128, 5'000'000},    // Small messages
        {256, 2'000'000},    // Medium messages
        {512, 1'000'000},    // Large messages
        {1024, 500'000},     // 1KB transactions
        {2048, 250'000},     // 2KB transactions
        {4096, 100'000},     // 4KB blocks
        {8192, 50'000},      // 8KB blocks
        {16384, 25'000},     // 16KB blocks
    };

    for (const auto& [size, iters] : sizes) {
        benchmark_serialize(size, iters);
        benchmark_deserialize(size, iters);
    }

    std::cout << "\nPGO profile data generated successfully!\n";
    std::cout << "Next: cmake .. -DENABLE_PGO_USE=ON && make\n";

    return 0;
}

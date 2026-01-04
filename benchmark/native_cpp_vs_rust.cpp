/**
 * Native C++ limcode benchmark (not FFI)
 * Comparing raw serialize/deserialize performance
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>

using namespace std::chrono;
using namespace limcode;

struct BenchResult {
    double serialize_gbps;
    double deserialize_gbps;
    size_t serialized_size;
};

BenchResult benchmark_size(size_t num_elements, size_t iterations) {
    const size_t data_bytes = num_elements * sizeof(uint64_t);
    const size_t buffer_size = data_bytes + 64;

    // 64-byte aligned allocation (like theoretical max benchmark)
    uint64_t* data = (uint64_t*)aligned_alloc(64, data_bytes);
    uint8_t* buffer = (uint8_t*)aligned_alloc(64, buffer_size);
    uint8_t* serialized = (uint8_t*)aligned_alloc(64, buffer_size);
    uint64_t* result = (uint64_t*)aligned_alloc(64, data_bytes);

    // Initialize data
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = 0xABCDEF0123456789ULL + i;
    }

    // Warmup
    for (size_t i = 0; i < 10; ++i) {
        *reinterpret_cast<uint64_t*>(buffer) = num_elements;
        limcode::limcode_memcpy_optimized(buffer + 8, data, data_bytes);
    }

    // Benchmark serialization
    auto ser_start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        *reinterpret_cast<uint64_t*>(buffer) = num_elements;
        limcode::limcode_memcpy_optimized(buffer + 8, data, data_bytes);

        // Prevent optimization
        volatile uint8_t sink = buffer[0];
        (void)sink;
    }

    auto ser_end = high_resolution_clock::now();
    double ser_ns = duration_cast<nanoseconds>(ser_end - ser_start).count() / (double)iterations;

    // Create serialized data for deserialization test
    *reinterpret_cast<uint64_t*>(serialized) = num_elements;
    limcode::limcode_memcpy_optimized(serialized + 8, data, data_bytes);

    // Benchmark deserialization
    auto deser_start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        size_t len;
        limcode::deserialize_pod_array(serialized, result, &len);

        // Prevent optimization
        volatile uint64_t sink = result[0];
        (void)sink;
    }

    auto deser_end = high_resolution_clock::now();
    double deser_ns = duration_cast<nanoseconds>(deser_end - deser_start).count() / (double)iterations;

    free(data);
    free(buffer);
    free(serialized);
    free(result);

    return BenchResult{
        .serialize_gbps = data_bytes / ser_ns,
        .deserialize_gbps = data_bytes / deser_ns,
        .serialized_size = buffer_size
    };
}

std::string format_size(size_t bytes) {
    if (bytes >= 1024 * 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024 * 1024)) + "GB";
    } else if (bytes >= 1024 * 1024) {
        return std::to_string(bytes / (1024 * 1024)) + "MB";
    } else if (bytes >= 1024) {
        return std::to_string(bytes / 1024) + "KB";
    } else {
        return std::to_string(bytes) + "B";
    }
}

int main() {
    std::cout << "\n🔥 NATIVE C++ LIMCODE BENCHMARK\n\n";

    struct TestConfig {
        size_t num_elements;
        const char* name;
        size_t iterations;
    };

    std::vector<TestConfig> configs = {
        {8,        "64B",    1000},
        {128,      "1KB",    1000},
        {1024,     "8KB",    500},
        {16384,    "128KB",  100},
        {131072,   "1MB",    50},
        {1048576,  "8MB",    10},
        {8388608,  "64MB",   3},
        {67108864, "512MB",  1},
    };

    std::cout << "| Size   | Serialize (GB/s) | Deserialize (GB/s) | Serialized Size |\n";
    std::cout << "|--------|------------------|--------------------|-----------------|--|\n";

    for (const auto& cfg : configs) {
        auto result = benchmark_size(cfg.num_elements, cfg.iterations);

        std::cout << "| " << std::setw(6) << cfg.name
                  << " | " << std::setw(16) << std::fixed << std::setprecision(2) << result.serialize_gbps
                  << " | " << std::setw(18) << result.deserialize_gbps
                  << " | " << std::setw(15) << format_size(result.serialized_size)
                  << " |\n";
    }

    std::cout << "\n";
    return 0;
}

/**
 * @file investigate_64mb.cpp
 * @brief Investigate why 64MB is 8.32 GiB/s instead of 12+ GiB/s
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace std::chrono;

void test_size(size_t num_elements, const char* label) {
    std::vector<uint64_t> data(num_elements);
    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = i;
    }

    size_t data_size = num_elements * sizeof(uint64_t);
    size_t iterations = std::max(size_t(10), 100000000 / (data_size + 1));

    std::vector<uint8_t> buf;

    // Warmup
    for (size_t i = 0; i < 10; ++i) {
        serialize_pod_into(buf, data);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        serialize_pod_into(buf, data);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count();
    double ns_per_op = ns / iterations;
    double gbps = (data_size / ns_per_op);

    std::cout << std::left << std::setw(20) << label
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << ns_per_op << " ns/op  "
              << std::setw(10) << std::setprecision(2) << gbps << " GiB/s\n";
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  Investigating 64MB Performance Gap\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    // Test around the 64MB size to find where performance drops
    test_size(1024 * 1024, "8MB");
    test_size(2 * 1024 * 1024, "16MB");
    test_size(4 * 1024 * 1024, "32MB");
    test_size(6 * 1024 * 1024, "48MB");
    test_size(8 * 1024 * 1024, "64MB");
    test_size(10 * 1024 * 1024, "80MB");
    test_size(12 * 1024 * 1024, "96MB");

    std::cout << "\n═══════════════════════════════════════════════════════════\n";
    std::cout << "Looking for performance cliff...\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";

    return 0;
}

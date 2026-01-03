// Overhead Analysis - Find the 7 GB/s gap
// Pure memcpy: 76 GB/s → Our serialize: 69 GB/s = 7 GB/s overhead

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>

using namespace std::chrono;

const size_t SIZE = 131072; // 128KB
const size_t ITERS = 1000;

// Test 1: Pure memcpy baseline (should be ~76 GB/s)
double test_pure_memcpy() {
    std::vector<uint8_t> src(SIZE, 0xAB);
    std::vector<uint8_t> dst(SIZE);

    // Warmup to ensure buffers are hot in cache
    for (int i = 0; i < 3; ++i) {
        __builtin_memcpy(dst.data(), src.data(), SIZE);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        __builtin_memcpy(dst.data(), src.data(), SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return SIZE / ns;
}

// Test 2: Memcpy + resize (measure resize overhead)
double test_memcpy_resize() {
    std::vector<uint8_t> src(SIZE, 0xAB);
    std::vector<uint8_t> dst;

    // Warmup
    for (int i = 0; i < 3; ++i) {
        dst.resize(SIZE + 8);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        dst.resize(SIZE + 8);
        __builtin_memcpy(dst.data() + 8, src.data(), SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return SIZE / ns;
}

// Test 3: Memcpy + resize + header write
double test_memcpy_resize_header() {
    std::vector<uint8_t> src(SIZE, 0xAB);
    std::vector<uint8_t> dst;

    // Warmup
    for (int i = 0; i < 3; ++i) {
        dst.resize(SIZE + 8);
        *reinterpret_cast<uint64_t*>(dst.data()) = SIZE;
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        dst.resize(SIZE + 8);
        *reinterpret_cast<uint64_t*>(dst.data()) = SIZE;
        __builtin_memcpy(dst.data() + 8, src.data(), SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return SIZE / ns;
}

// Test 4: Full serialize (matching table_bench.cpp pattern)
double test_full_serialize() {
    std::vector<uint64_t> data(SIZE / 8, 0xABCDEF);
    std::vector<uint8_t> buf;

    // Warmup
    for (int i = 0; i < 3; ++i) {
        const size_t count = data.size();
        const size_t bytes = count * sizeof(uint64_t);
        const size_t total = 8 + bytes;
        if (buf.capacity() < total) buf.reserve(total);
        buf.resize(total);
        uint8_t* ptr = buf.data();
        *reinterpret_cast<uint64_t*>(ptr) = count;
        __builtin_memcpy(ptr + 8, data.data(), bytes);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
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

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return SIZE / ns;
}

// Test 5: Optimized - no resize, use set_len trick
double test_optimized_no_resize() {
    std::vector<uint64_t> data(SIZE / 8, 0xABCDEF);
    std::vector<uint8_t> buf;

    const size_t total = SIZE + 8;
    buf.reserve(total);

    // Warmup
    for (int i = 0; i < 3; ++i) {
        buf.resize(total);
        uint8_t* ptr = buf.data();
        *reinterpret_cast<uint64_t*>(ptr) = SIZE / 8;
        __builtin_memcpy(ptr + 8, data.data(), SIZE);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        // TRICK: Manually set size without resize initialization
        // This mimics Rust's Vec::set_len()
        struct VecHack {
            uint8_t* ptr;
            size_t len;
            size_t cap;
        };
        auto* hack = reinterpret_cast<VecHack*>(&buf);
        hack->len = total;  // Directly set length, skip resize initialization

        uint8_t* ptr = buf.data();
        *reinterpret_cast<uint64_t*>(ptr) = SIZE / 8;
        __builtin_memcpy(ptr + 8, data.data(), SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return SIZE / ns;
}

// Test 6: Exact table_bench pattern (inline)
double test_table_bench_pattern() {
    const size_t num_elements = SIZE / 8;
    const size_t data_size = num_elements * sizeof(uint64_t);

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

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
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

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return data_size / ns;
}

int main() {
    std::cout << "Overhead Analysis (128KB)\n";
    std::cout << "=========================\n\n";

    double pure = test_pure_memcpy();
    double resize = test_memcpy_resize();
    double header = test_memcpy_resize_header();
    double full = test_full_serialize();
    double optimized = test_optimized_no_resize();
    double table = test_table_bench_pattern();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "1. Pure memcpy:             " << pure << " GB/s (baseline)\n";
    std::cout << "2. + resize():              " << resize << " GB/s (-" << (pure - resize) << " GB/s)\n";
    std::cout << "3. + header write:          " << header << " GB/s (-" << (pure - header) << " GB/s)\n";
    std::cout << "4. + conditional check:     " << full << " GB/s (-" << (pure - full) << " GB/s)\n";
    std::cout << "5. Optimized (no resize):   " << optimized << " GB/s (-" << (pure - optimized) << " GB/s)\n";
    std::cout << "6. Table_bench pattern:     " << table << " GB/s (-" << (pure - table) << " GB/s) <- SHOULD MATCH TABLE_BENCH\n";

    std::cout << "\nOverhead breakdown:\n";
    std::cout << "  resize() cost:            " << std::setw(6) << (pure - resize) << " GB/s\n";
    std::cout << "  header write cost:        " << std::setw(6) << (resize - header) << " GB/s\n";
    std::cout << "  conditional check cost:   " << std::setw(6) << (header - full) << " GB/s\n";
    std::cout << "  Total overhead:           " << std::setw(6) << (pure - full) << " GB/s\n";
    std::cout << "\n  Optimized savings:        " << std::setw(6) << (optimized - full) << " GB/s\n";
    std::cout << "  Table_bench efficiency:   " << std::setw(6) << (table / pure * 100) << "%\n";

    return 0;
}

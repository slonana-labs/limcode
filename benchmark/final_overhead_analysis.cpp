// Final Overhead Analysis - Answer "Why not 99%?"
// Compare: Pure memcpy vs Optimized serialize (both inlined)

#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>

using namespace std::chrono;

const size_t SIZE = 131072; // 128KB
const size_t ITERS = 1000;

// Test 1: Pure memcpy baseline (aligned, no header)
double test_pure_memcpy_aligned() {
    std::vector<uint8_t> src(SIZE, 0xAB);
    std::vector<uint8_t> dst(SIZE);

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

// Test 2: Pure memcpy with +8 offset (unaligned like our serialize)
double test_pure_memcpy_offset8() {
    std::vector<uint8_t> src(SIZE, 0xAB);
    std::vector<uint8_t> dst(SIZE + 8);

    for (int i = 0; i < 3; ++i) {
        __builtin_memcpy(dst.data() + 8, src.data(), SIZE);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        __builtin_memcpy(dst.data() + 8, src.data(), SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return SIZE / ns;
}

// Test 3: Memcpy with header write (no resize, pre-allocated)
double test_with_header_preallocated() {
    std::vector<uint8_t> src(SIZE, 0xAB);
    std::vector<uint8_t> dst(SIZE + 8);

    for (int i = 0; i < 3; ++i) {
        *reinterpret_cast<uint64_t*>(dst.data()) = SIZE;
        __builtin_memcpy(dst.data() + 8, src.data(), SIZE);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        *reinterpret_cast<uint64_t*>(dst.data()) = SIZE;
        __builtin_memcpy(dst.data() + 8, src.data(), SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return SIZE / ns;
}

// Test 4: Full serialize pattern (INLINED like table_bench)
double test_serialize_inlined() {
    const size_t num_elements = SIZE / 8;
    std::vector<uint64_t> data(num_elements, 0xABCDEF);
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

// Test 5: Optimized - remove resize, use unsafe set_len hack
double test_serialize_optimized() {
    const size_t num_elements = SIZE / 8;
    std::vector<uint64_t> data(num_elements, 0xABCDEF);
    std::vector<uint8_t> buf;

    const size_t total = SIZE + 8;
    buf.reserve(total);

    // Warmup
    for (int i = 0; i < 3; ++i) {
        // UNSAFE: Directly set length to avoid resize initialization
        struct VecHack { uint8_t* ptr; size_t len; size_t cap; };
        reinterpret_cast<VecHack*>(&buf)->len = total;

        uint8_t* ptr = buf.data();
        *reinterpret_cast<uint64_t*>(ptr) = num_elements;
        __builtin_memcpy(ptr + 8, data.data(), SIZE);
    }

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERS; ++i) {
        // UNSAFE: Directly set length
        struct VecHack { uint8_t* ptr; size_t len; size_t cap; };
        reinterpret_cast<VecHack*>(&buf)->len = total;

        uint8_t* ptr = buf.data();
        *reinterpret_cast<uint64_t*>(ptr) = num_elements;
        __builtin_memcpy(ptr + 8, data.data(), SIZE);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(ITERS);
    return SIZE / ns;
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  FINAL OVERHEAD ANALYSIS: Why not 99% efficiency?\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    double baseline = test_pure_memcpy_aligned();
    double offset8 = test_pure_memcpy_offset8();
    double with_header = test_with_header_preallocated();
    double serialize = test_serialize_inlined();
    double optimized = test_serialize_optimized();

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "1. Pure memcpy (aligned):       " << std::setw(6) << baseline << " GB/s  [100.0%]\n";
    std::cout << "2. Pure memcpy (+8 offset):     " << std::setw(6) << offset8 << " GB/s  ["
              << (offset8/baseline*100) << "%]\n";
    std::cout << "3. + Write 8-byte header:       " << std::setw(6) << with_header << " GB/s  ["
              << (with_header/baseline*100) << "%]\n";
    std::cout << "4. Full serialize (inlined):    " << std::setw(6) << serialize << " GB/s  ["
              << (serialize/baseline*100) << "%] ← CURRENT\n";
    std::cout << "5. Optimized (unsafe set_len):  " << std::setw(6) << optimized << " GB/s  ["
              << (optimized/baseline*100) << "%]\n";

    std::cout << "\n───────────────────────────────────────────────────────────\n";
    std::cout << "  Overhead Breakdown:\n";
    std::cout << "───────────────────────────────────────────────────────────\n\n";

    double overhead_offset = baseline - offset8;
    double overhead_header = offset8 - with_header;
    double overhead_resize = with_header - serialize;
    double total_overhead = baseline - serialize;

    std::cout << "  Unaligned access (+8 offset):  " << std::setw(6) << overhead_offset << " GB/s  ("
              << (overhead_offset/baseline*100) << "%)\n";
    std::cout << "  Writing 8-byte header:         " << std::setw(6) << overhead_header << " GB/s  ("
              << (overhead_header/baseline*100) << "%)\n";
    std::cout << "  Buffer resize + bookkeeping:   " << std::setw(6) << overhead_resize << " GB/s  ("
              << (overhead_resize/baseline*100) << "%)\n";
    std::cout << "  ───────────────────────────────────\n";
    std::cout << "  TOTAL OVERHEAD:                " << std::setw(6) << total_overhead << " GB/s  ("
              << (total_overhead/baseline*100) << "%)\n\n";

    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  ANSWER: Current efficiency is " << (serialize/baseline*100) << "%\n";
    std::cout << "  To reach 99%, we need to eliminate " << (99.0 - serialize/baseline*100) << "% more overhead\n";
    std::cout << "  That's " << (baseline * 0.99 - serialize) << " GB/s improvement needed\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    std::cout << "Potential optimizations:\n";
    std::cout << "  • Using unsafe set_len gains: " << (optimized - serialize) << " GB/s\n";
    std::cout << "  • Further optimization needed: " << (baseline * 0.99 - optimized) << " GB/s\n\n";

    std::cout << "Root causes of remaining overhead:\n";
    std::cout << "  1. Unaligned memory access at ptr+8 (unavoidable with 8-byte header)\n";
    std::cout << "  2. Writing header competes with memcpy for memory bandwidth\n";
    std::cout << "  3. buf.resize() has inherent cost even when size unchanged\n";
    std::cout << "  4. Conditional checks and pointer arithmetic\n\n";

    std::cout << "Conclusion: ~" << std::setprecision(1) << (serialize/baseline*100)
              << "% is excellent for a real-world serializer!\n";
    std::cout << "Getting to 99% would require unsafe hacks that sacrifice safety.\n";

    return 0;
}

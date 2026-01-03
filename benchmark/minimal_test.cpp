// ULTRA MINIMAL - find the bottleneck

#include <chrono>
#include <iostream>
#include <vector>
#include <cstring>

using namespace std::chrono;

// Test 1: Just memcpy (baseline)
double test_raw_memcpy(size_t size, size_t iters) {
    std::vector<uint8_t> src(size, 0xAB);
    std::vector<uint8_t> dst(size);

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iters; ++i) {
        __builtin_memcpy(dst.data(), src.data(), size);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(iters);
    return size / ns;
}

// Test 2: memcpy + write 8-byte header
double test_with_header(size_t size, size_t iters) {
    std::vector<uint8_t> src(size, 0xAB);
    std::vector<uint8_t> dst(size + 8);

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iters; ++i) {
        *reinterpret_cast<uint64_t*>(dst.data()) = size;
        __builtin_memcpy(dst.data() + 8, src.data(), size);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(iters);
    return size / ns;
}

// Test 3: Full serialize (current implementation)
template<typename T>
void serialize(std::vector<uint8_t>& buf, const std::vector<T>& data) {
    const size_t count = data.size();
    const size_t data_bytes = count * sizeof(T);
    const size_t total_size = 8 + data_bytes;

    if (buf.capacity() < total_size) buf.reserve(total_size);
    buf.resize(total_size);

    uint8_t* ptr = buf.data();
    *reinterpret_cast<uint64_t*>(ptr) = count;
    __builtin_memcpy(ptr + 8, data.data(), data_bytes);
}

double test_full_serialize(size_t size, size_t iters) {
    std::vector<uint64_t> data(size / 8);
    std::vector<uint8_t> buf;

    // Warmup
    for (int i = 0; i < 3; ++i) serialize(buf, data);

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iters; ++i) {
        serialize(buf, data);
    }
    auto end = high_resolution_clock::now();

    double ns = duration_cast<nanoseconds>(end - start).count() / double(iters);
    return size / ns;
}

int main() {
    const size_t size = 131072; // 128KB
    const size_t iters = 1000;

    std::cout << "128KB Performance Test:\n";
    std::cout << "Raw memcpy:        " << test_raw_memcpy(size, iters) << " GB/s\n";
    std::cout << "With header:       " << test_with_header(size, iters) << " GB/s\n";
    std::cout << "Full serialize:    " << test_full_serialize(size, iters) << " GB/s\n";

    return 0;
}

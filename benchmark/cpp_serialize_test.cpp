/**
 * @file cpp_serialize_test.cpp
 * @brief C++ limcode serialization test - outputs binary for comparison with wincode
 */

#include <limcode/limcode.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

void write_test_case(const char* filename, const std::vector<uint64_t>& data) {
    std::vector<uint8_t> buf;
    limcode::serialize_pod_into(buf, data);

    std::ofstream out(filename, std::ios::binary);
    out.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    out.close();

    std::cout << filename << ": " << buf.size() << " bytes written\n";
}

int main() {
    std::cout << "C++ limcode serialization test\n\n";

    // Test 1: 1KB of u64 (128 elements)
    write_test_case("/tmp/limcode_1kb.bin", std::vector<uint64_t>(128, 0xABCDEF0123456789ULL));

    // Test 2: 8KB of u64 (1024 elements)
    write_test_case("/tmp/limcode_8kb.bin", std::vector<uint64_t>(1024, 0xABCDEF0123456789ULL));

    // Test 3: Empty vector
    write_test_case("/tmp/limcode_empty.bin", std::vector<uint64_t>());

    // Test 4: Single element
    write_test_case("/tmp/limcode_single.bin", std::vector<uint64_t>(1, 42));

    // Test 5: Sequential data
    std::vector<uint64_t> sequential(1000);
    for (size_t i = 0; i < 1000; ++i) {
        sequential[i] = i;
    }
    write_test_case("/tmp/limcode_sequential.bin", sequential);

    std::cout << "\nAll test cases written to /tmp/limcode_*.bin\n";
    return 0;
}

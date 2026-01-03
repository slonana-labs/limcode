/**
 * @file cpp_bincode_compat.cpp
 * @brief Test that C++ limcode produces bincode-compatible output
 */

#include <limcode/limcode.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>

using namespace limcode;

// Reference bincode output for Vec<u64> [0..9]
const uint8_t REFERENCE_VEC_U64_10[] = {
    0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // len = 10
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 1
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 2
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 3
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 4
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 5
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 6
    0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 7
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 8
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 9
};

int main() {
    std::cout << "═══════════════════════════════════════════════════════════\n";
    std::cout << "  C++ Bincode Compatibility Test\n";
    std::cout << "═══════════════════════════════════════════════════════════\n\n";

    std::vector<uint64_t> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    LimcodeEncoder enc;
    enc.write_u64(data.size());
    enc.write_bytes(reinterpret_cast<const uint8_t*>(data.data()), data.size() * 8);
    auto cpp_bytes = std::move(enc).finish();

    if (cpp_bytes.size() != sizeof(REFERENCE_VEC_U64_10)) {
        std::cerr << "❌ Size mismatch!\n";
        return 1;
    }

    if (std::memcmp(cpp_bytes.data(), REFERENCE_VEC_U64_10, sizeof(REFERENCE_VEC_U64_10)) != 0) {
        std::cerr << "❌ Byte mismatch!\n";
        return 1;
    }

    std::cout << "✅ C++ output matches bincode reference!\n";
    std::cout << "═══════════════════════════════════════════════════════════\n";
    return 0;
}

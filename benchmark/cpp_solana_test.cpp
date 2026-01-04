/**
 * @file cpp_solana_test.cpp
 * @brief Test C++ limcode with real Solana transaction data
 */

#include <limcode/limcode.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

std::vector<uint8_t> read_file(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error(std::string("Failed to open ") + filename);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error(std::string("Failed to read ") + filename);
    }

    return buffer;
}

void test_solana_tx() {
    std::cout << "Testing C++ limcode with real Solana transaction\n\n";

    // Read the bincode-serialized Solana transaction
    auto tx_bincode = read_file("/tmp/solana_tx_bincode.bin");
    std::cout << "✓ Read Solana transaction: " << tx_bincode.size() << " bytes\n";

    // Show first 32 bytes
    std::cout << "  First 32 bytes: [";
    for (size_t i = 0; i < 32 && i < tx_bincode.size(); ++i) {
        if (i > 0) std::cout << ", ";
        printf("%02x", tx_bincode[i]);
    }
    std::cout << "]\n\n";

    // Now serialize this byte array with limcode
    // When we serialize Vec<u8> with bincode, it adds 8-byte length prefix
    std::vector<uint8_t> limcode_output;
    limcode::serialize_pod_into(limcode_output, tx_bincode);

    std::cout << "✓ Limcode serialized: " << limcode_output.size() << " bytes\n";
    std::cout << "  Expected: " << (tx_bincode.size() + 8) << " bytes (tx + 8-byte length)\n";

    // Verify format: first 8 bytes should be length
    uint64_t length = *reinterpret_cast<uint64_t*>(limcode_output.data());
    std::cout << "  Length header: " << length << " (0x" << std::hex << length << std::dec << ")\n";

    if (length == tx_bincode.size()) {
        std::cout << "  ✓ Length header correct!\n";
    } else {
        std::cout << "  ✗ Length mismatch! Expected " << tx_bincode.size() << "\n";
        return;
    }

    // Verify data matches
    bool data_matches = true;
    for (size_t i = 0; i < tx_bincode.size(); ++i) {
        if (limcode_output[i + 8] != tx_bincode[i]) {
            std::cout << "  ✗ Data mismatch at byte " << i << ": "
                      << "limcode=" << (int)limcode_output[i + 8]
                      << " vs original=" << (int)tx_bincode[i] << "\n";
            data_matches = false;
            break;
        }
    }

    if (data_matches) {
        std::cout << "  ✓ All data bytes match!\n";
        std::cout << "\n✅ C++ limcode correctly handles real Solana transaction data!\n";
    } else {
        std::cout << "\n❌ Data mismatch detected!\n";
    }
}

int main() {
    try {
        test_solana_tx();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

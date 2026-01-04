/**
 * @file cpp_solana_bench.cpp
 * @brief C++ limcode benchmark with Solana-sized transaction data
 */

#include <limcode/limcode.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdint>

using namespace std::chrono;

// Simulate Solana transaction structure:
// - Signature: 64 bytes
// - Message header: 3 bytes
// - Account keys: variable (32 bytes each)
// - Recent blockhash: 32 bytes
// - Instructions: variable
struct SolanaTransaction {
    std::vector<uint8_t> signature;     // 64 bytes
    std::vector<uint8_t> message;       // Variable size

    SolanaTransaction(size_t num_accounts, size_t num_instructions, size_t instruction_data_size) {
        // Signature
        signature.resize(64, 0xAB);

        // Message structure
        message.reserve(1024);

        // Header (3 bytes)
        message.push_back(1);  // num_required_signatures
        message.push_back(0);  // num_readonly_signed_accounts
        message.push_back(1);  // num_readonly_unsigned_accounts

        // Account keys length (compact-u16)
        message.push_back(static_cast<uint8_t>(num_accounts));

        // Account keys (32 bytes each)
        for (size_t i = 0; i < num_accounts; ++i) {
            for (int j = 0; j < 32; ++j) {
                message.push_back(static_cast<uint8_t>(i + j));
            }
        }

        // Recent blockhash (32 bytes)
        for (int i = 0; i < 32; ++i) {
            message.push_back(0x42);
        }

        // Instructions length
        message.push_back(static_cast<uint8_t>(num_instructions));

        // Instructions
        for (size_t i = 0; i < num_instructions; ++i) {
            message.push_back(2);  // program_id_index
            message.push_back(2);  // accounts length
            message.push_back(0);  // account index 0
            message.push_back(1);  // account index 1

            // Instruction data length
            message.push_back(static_cast<uint8_t>(instruction_data_size));

            // Instruction data
            for (size_t j = 0; j < instruction_data_size; ++j) {
                message.push_back(static_cast<uint8_t>(i + j));
            }
        }
    }

    std::vector<uint8_t> serialize_bincode() const {
        std::vector<uint8_t> result;

        // Signature count (1)
        result.push_back(1);

        // Signature
        result.insert(result.end(), signature.begin(), signature.end());

        // Message
        result.insert(result.end(), message.begin(), message.end());

        return result;
    }
};

double benchmark_tx(const SolanaTransaction& tx, size_t iterations, const char* label) {
    auto serialized = tx.serialize_bincode();
    size_t tx_size = serialized.size();

    std::vector<uint8_t> output;

    // Warmup
    for (size_t i = 0; i < 10; ++i) {
        limcode::serialize_pod_into(output, serialized);
    }

    // Benchmark
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        limcode::serialize_pod_into(output, serialized);
        volatile uint8_t sink = output[0];
        (void)sink;
    }
    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / static_cast<double>(iterations);
    double throughput_gbps = (tx_size / ns_per_op);

    std::cout << std::left << std::setw(35) << label
              << std::right << std::setw(10) << tx_size << " bytes  "
              << std::setw(12) << std::fixed << std::setprecision(2)
              << throughput_gbps << " GB/s  "
              << std::setw(10) << std::fixed << std::setprecision(2)
              << (ns_per_op) << " ns/op\n";

    return throughput_gbps;
}

int main() {
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  C++ Limcode Benchmark: Solana Transaction Patterns\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n\n";

    std::cout << std::left << std::setw(35) << "Transaction Type"
              << std::right << std::setw(10) << "Size" << "        "
              << std::setw(12) << "Throughput" << "  "
              << std::setw(10) << "Latency" << "\n";
    std::cout << std::string(75, '-') << "\n";

    // Simple transfer (3 accounts, 1 instruction, 12 bytes data)
    SolanaTransaction simple_transfer(3, 1, 12);
    benchmark_tx(simple_transfer, 1000000, "Simple transfer");

    // Token transfer (4 accounts, 1 instruction, 16 bytes data)
    SolanaTransaction token_transfer(4, 1, 16);
    benchmark_tx(token_transfer, 500000, "Token transfer");

    // Swap transaction (6 accounts, 1 instruction, 32 bytes data)
    SolanaTransaction swap_tx(6, 1, 32);
    benchmark_tx(swap_tx, 500000, "Swap transaction");

    // Complex DeFi (10 accounts, 3 instructions, 64 bytes each)
    SolanaTransaction defi_tx(10, 3, 64);
    benchmark_tx(defi_tx, 250000, "Complex DeFi (3 instructions)");

    // Very complex (15 accounts, 5 instructions, 64 bytes each)
    SolanaTransaction very_complex(15, 5, 64);
    benchmark_tx(very_complex, 100000, "Very complex (5 instructions)");

    // NFT mint (8 accounts, 2 instructions, 128 bytes data)
    SolanaTransaction nft_mint(8, 2, 128);
    benchmark_tx(nft_mint, 200000, "NFT mint");

    // Large transaction (20 accounts, 10 instructions, 128 bytes each)
    SolanaTransaction large_tx(20, 10, 128);
    benchmark_tx(large_tx, 50000, "Large tx (10 instructions)");

    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  Note: Throughput = bytes_processed / time (higher is better)\n";
    std::cout << "  Real Solana transactions: 200-400 bytes (simple) to 1KB+ (complex)\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n\n";

    return 0;
}

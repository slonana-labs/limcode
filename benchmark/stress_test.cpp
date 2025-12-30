/**
 * @file stress_test.cpp
 * @brief Realistic Solana load simulation
 *
 * Simulates continuous block production at Solana's 400ms slot time.
 * Tests: Can we serialize fast enough to keep up with real validator load?
 */

#include <limcode/limcode.h>
#include <limcode/wincode.h>
#include <limcode/bincode.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <atomic>
#include <numeric>

using namespace std::chrono;
using namespace limcode;

// ==================== Realistic Data Generator ====================

class SolanaLoadGenerator {
public:
  explicit SolanaLoadGenerator(uint32_t seed = 42) : rng_(seed) {}

  // Vote transaction (70% of Solana traffic)
  Entry generate_vote() {
    Entry e;
    e.num_hashes = dist_(rng_) % 500 + 1;
    e.hash = random_hash();

    VersionedTransaction tx;
    tx.signatures.push_back(random_signature());

    LegacyMessage msg;
    msg.header = {1, 1, 5};  // 1 signer, 1 readonly signed, 5 readonly unsigned

    // Vote account, validator identity, clock sysvar, slot hashes, etc.
    for (int i = 0; i < 7; ++i) {
      msg.account_keys.push_back(random_hash());
    }
    msg.recent_blockhash = random_hash();

    // Vote instruction with typical vote data
    CompiledInstruction instr;
    instr.program_id_index = 6;
    instr.accounts = {0, 1, 2, 3, 4, 5};
    instr.data = random_bytes(44);  // Typical vote instruction size
    msg.instructions.push_back(std::move(instr));

    tx.message.set_legacy(std::move(msg));
    e.transactions.push_back(std::move(tx));
    return e;
  }

  // Simple transfer (15% of traffic)
  Entry generate_transfer() {
    Entry e;
    e.num_hashes = dist_(rng_) % 200 + 1;
    e.hash = random_hash();

    VersionedTransaction tx;
    tx.signatures.push_back(random_signature());

    LegacyMessage msg;
    msg.header = {1, 0, 1};
    msg.account_keys = {random_hash(), random_hash(), random_hash()};
    msg.recent_blockhash = random_hash();

    CompiledInstruction instr;
    instr.program_id_index = 2;
    instr.accounts = {0, 1};
    instr.data = random_bytes(12);  // Transfer lamports
    msg.instructions.push_back(std::move(instr));

    tx.message.set_legacy(std::move(msg));
    e.transactions.push_back(std::move(tx));
    return e;
  }

  // Token transfer (8% of traffic)
  Entry generate_token_transfer() {
    Entry e;
    e.num_hashes = dist_(rng_) % 300 + 1;
    e.hash = random_hash();

    VersionedTransaction tx;
    tx.signatures.push_back(random_signature());

    LegacyMessage msg;
    msg.header = {1, 0, 4};
    for (int i = 0; i < 6; ++i) {
      msg.account_keys.push_back(random_hash());
    }
    msg.recent_blockhash = random_hash();

    CompiledInstruction instr;
    instr.program_id_index = 5;
    instr.accounts = {0, 1, 2, 3, 4};
    instr.data = random_bytes(9);  // SPL token transfer
    msg.instructions.push_back(std::move(instr));

    tx.message.set_legacy(std::move(msg));
    e.transactions.push_back(std::move(tx));
    return e;
  }

  // DeFi swap with ALT (5% of traffic)
  Entry generate_defi_swap() {
    Entry e;
    e.num_hashes = dist_(rng_) % 100 + 1;
    e.hash = random_hash();

    VersionedTransaction tx;
    tx.signatures.push_back(random_signature());
    tx.signatures.push_back(random_signature());  // Often 2 signers

    V0Message msg;
    msg.header = {2, 0, 8};
    for (int i = 0; i < 12; ++i) {
      msg.account_keys.push_back(random_hash());
    }
    msg.recent_blockhash = random_hash();

    // Multiple instructions for swap
    for (int i = 0; i < 4; ++i) {
      CompiledInstruction instr;
      instr.program_id_index = static_cast<uint8_t>(8 + i);
      instr.accounts = {0, 1, 2, 3, 4, 5, 6, 7};
      instr.data = random_bytes(64 + (dist_(rng_) % 128));
      msg.instructions.push_back(std::move(instr));
    }

    // Address table lookups
    for (int i = 0; i < 2; ++i) {
      AddressTableLookup atl;
      atl.account_key = random_hash();
      atl.writable_indexes = {0, 1, 2, 3};
      atl.readonly_indexes = {4, 5, 6, 7, 8};
      msg.address_table_lookups.push_back(std::move(atl));
    }

    tx.message.set_v0(std::move(msg));
    e.transactions.push_back(std::move(tx));
    return e;
  }

  // NFT mint/transfer (2% of traffic)
  Entry generate_nft() {
    Entry e;
    e.num_hashes = dist_(rng_) % 50 + 1;
    e.hash = random_hash();

    VersionedTransaction tx;
    tx.signatures.push_back(random_signature());

    V0Message msg;
    msg.header = {1, 0, 6};
    for (int i = 0; i < 10; ++i) {
      msg.account_keys.push_back(random_hash());
    }
    msg.recent_blockhash = random_hash();

    // Multiple metaplex instructions
    for (int i = 0; i < 3; ++i) {
      CompiledInstruction instr;
      instr.program_id_index = static_cast<uint8_t>(7 + i);
      instr.accounts = {0, 1, 2, 3, 4, 5};
      instr.data = random_bytes(200 + (dist_(rng_) % 300));  // NFT metadata
      msg.instructions.push_back(std::move(instr));
    }

    tx.message.set_v0(std::move(msg));
    e.transactions.push_back(std::move(tx));
    return e;
  }

  // Generate realistic Solana block
  std::vector<Entry> generate_block(size_t num_entries) {
    std::vector<Entry> entries;
    entries.reserve(num_entries);

    std::uniform_int_distribution<int> type_dist(1, 100);

    for (size_t i = 0; i < num_entries; ++i) {
      int roll = type_dist(rng_);
      if (roll <= 70) {
        entries.push_back(generate_vote());
      } else if (roll <= 85) {
        entries.push_back(generate_transfer());
      } else if (roll <= 93) {
        entries.push_back(generate_token_transfer());
      } else if (roll <= 98) {
        entries.push_back(generate_defi_swap());
      } else {
        entries.push_back(generate_nft());
      }
    }
    return entries;
  }

private:
  std::mt19937 rng_;
  std::uniform_int_distribution<uint64_t> dist_{0, UINT64_MAX};

  std::array<uint8_t, 32> random_hash() {
    std::array<uint8_t, 32> h;
    for (auto& b : h) b = static_cast<uint8_t>(dist_(rng_) & 0xFF);
    return h;
  }

  std::array<uint8_t, 64> random_signature() {
    std::array<uint8_t, 64> s;
    for (auto& b : s) b = static_cast<uint8_t>(dist_(rng_) & 0xFF);
    return s;
  }

  std::vector<uint8_t> random_bytes(size_t n) {
    std::vector<uint8_t> v(n);
    for (auto& b : v) b = static_cast<uint8_t>(dist_(rng_) & 0xFF);
    return v;
  }
};

// ==================== Stress Tests ====================

void test_sustained_load(size_t entries_per_block, int duration_seconds) {
  std::cout << "\n=== Sustained Load Test: " << entries_per_block
            << " entries/block for " << duration_seconds << "s ===\n";

  SolanaLoadGenerator gen;

  // Pre-generate blocks to avoid generator overhead in timing
  std::vector<std::vector<Entry>> blocks;
  int num_blocks = (duration_seconds * 1000) / 400 + 10;  // 400ms slots
  for (int i = 0; i < num_blocks; ++i) {
    blocks.push_back(gen.generate_block(entries_per_block));
  }

  std::cout << "Pre-generated " << num_blocks << " blocks\n\n";

  // Test each serializer
  for (const char* name : {"Limcode", "Wincode", "Bincode"}) {
    std::vector<double> block_times_us;
    size_t total_bytes = 0;
    int blocks_processed = 0;

    auto test_start = high_resolution_clock::now();
    auto deadline = test_start + seconds(duration_seconds);

    while (high_resolution_clock::now() < deadline && blocks_processed < num_blocks) {
      auto& block = blocks[blocks_processed % blocks.size()];

      auto start = high_resolution_clock::now();

      if (std::string(name) == "Limcode") {
        auto span = limcode::serialize(block);
        total_bytes += span.size();
      } else if (std::string(name) == "Wincode") {
        auto bytes = wincode::serialize(block);
        total_bytes += bytes.size();
      } else {
        auto bytes = bincode::serialize(block);
        total_bytes += bytes.size();
      }

      auto end = high_resolution_clock::now();
      double us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
      block_times_us.push_back(us);
      blocks_processed++;
    }

    auto test_end = high_resolution_clock::now();
    double total_ms = duration_cast<microseconds>(test_end - test_start).count() / 1000.0;

    // Calculate statistics
    std::sort(block_times_us.begin(), block_times_us.end());
    double avg_us = std::accumulate(block_times_us.begin(), block_times_us.end(), 0.0) / block_times_us.size();
    double p50_us = block_times_us[block_times_us.size() / 2];
    double p95_us = block_times_us[static_cast<size_t>(block_times_us.size() * 0.95)];
    double p99_us = block_times_us[static_cast<size_t>(block_times_us.size() * 0.99)];
    double max_us = block_times_us.back();

    double blocks_per_sec = blocks_processed / (total_ms / 1000.0);
    double throughput_gbps = (total_bytes * 8.0) / (total_ms * 1e6);
    double avg_block_kb = (total_bytes / blocks_processed) / 1024.0;

    // Can we keep up with 400ms slots?
    double slot_budget_us = 400000.0;  // 400ms in microseconds
    double headroom_pct = ((slot_budget_us - avg_us) / slot_budget_us) * 100.0;
    bool can_keep_up = p99_us < slot_budget_us;

    std::cout << std::fixed << std::setprecision(1);
    std::cout << name << ":\n";
    std::cout << "  Blocks: " << blocks_processed << " in " << (total_ms/1000.0) << "s\n";
    std::cout << "  Rate: " << std::setprecision(0) << blocks_per_sec << " blocks/s, "
              << std::setprecision(1) << throughput_gbps << " Gbps\n";
    std::cout << "  Block size: " << avg_block_kb << " KB avg\n";
    std::cout << "  Latency (us): avg=" << avg_us << ", p50=" << p50_us
              << ", p95=" << p95_us << ", p99=" << p99_us << ", max=" << max_us << "\n";
    std::cout << "  400ms slot headroom: " << headroom_pct << "% "
              << (can_keep_up ? "(OK)" : "(CANNOT KEEP UP!)") << "\n\n";
  }
}

void test_burst_load() {
  std::cout << "\n=== Burst Load Test (max throughput) ===\n";

  SolanaLoadGenerator gen;

  for (size_t entries : {1000, 2000, 5000, 10000, 20000}) {
    auto block = gen.generate_block(entries);

    // Warmup
    for (int i = 0; i < 5; ++i) {
      auto span = limcode::serialize(block);
      (void)span.size();
    }

    constexpr int ITERATIONS = 100;

    // Limcode
    auto lim_start = high_resolution_clock::now();
    size_t lim_bytes = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
      auto span = limcode::serialize(block);
      lim_bytes += span.size();
    }
    auto lim_end = high_resolution_clock::now();
    double lim_us = duration_cast<nanoseconds>(lim_end - lim_start).count() / 1000.0;

    // Wincode
    auto win_start = high_resolution_clock::now();
    size_t win_bytes = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
      auto bytes = wincode::serialize(block);
      win_bytes += bytes.size();
    }
    auto win_end = high_resolution_clock::now();
    double win_us = duration_cast<nanoseconds>(win_end - win_start).count() / 1000.0;

    // Bincode
    auto bin_start = high_resolution_clock::now();
    size_t bin_bytes = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
      auto bytes = bincode::serialize(block);
      bin_bytes += bytes.size();
    }
    auto bin_end = high_resolution_clock::now();
    double bin_us = duration_cast<nanoseconds>(bin_end - bin_start).count() / 1000.0;

    double lim_block_us = lim_us / ITERATIONS;
    double win_block_us = win_us / ITERATIONS;
    double bin_block_us = bin_us / ITERATIONS;

    double lim_vs_win = win_block_us / lim_block_us;
    double lim_vs_bin = bin_block_us / lim_block_us;

    std::cout << std::fixed << std::setprecision(0);
    std::cout << entries << " entries: Limcode " << lim_block_us << "us, "
              << "Wincode " << win_block_us << "us, "
              << "Bincode " << bin_block_us << "us "
              << "-> Limcode " << std::setprecision(2) << lim_vs_win << "x vs Win, "
              << lim_vs_bin << "x vs Bin\n";
  }
}

void test_memory_pressure() {
  std::cout << "\n=== Memory Pressure Test ===\n";
  std::cout << "Simulating validator with limited memory bandwidth...\n\n";

  SolanaLoadGenerator gen;
  auto block = gen.generate_block(5000);

  // Allocate large buffer to create memory pressure
  std::vector<uint8_t> pressure_buffer(512 * 1024 * 1024);  // 512MB
  for (size_t i = 0; i < pressure_buffer.size(); i += 4096) {
    pressure_buffer[i] = static_cast<uint8_t>(i);
  }

  constexpr int ITERATIONS = 50;

  for (const char* name : {"Limcode", "Wincode"}) {
    // Touch pressure buffer to pollute cache
    volatile uint8_t sum = 0;
    for (size_t i = 0; i < pressure_buffer.size(); i += 64) {
      sum += pressure_buffer[i];
    }
    (void)sum;

    auto start = high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; ++i) {
      if (std::string(name) == "Limcode") {
        auto span = limcode::serialize(block);
        (void)span.size();
      } else {
        auto bytes = wincode::serialize(block);
        (void)bytes.size();
      }

      // Pollute cache between iterations
      for (size_t j = 0; j < pressure_buffer.size(); j += 4096) {
        pressure_buffer[j]++;
      }
    }
    auto end = high_resolution_clock::now();

    double total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    double per_block_ms = total_ms / ITERATIONS;

    std::cout << name << ": " << std::fixed << std::setprecision(2)
              << per_block_ms << " ms/block under memory pressure\n";
  }
}

int main() {
  std::cout << "\n";
  std::cout << "================================================================\n";
  std::cout << "       SOLANA REALISTIC LOAD SIMULATION\n";
  std::cout << "================================================================\n";
  std::cout << "Traffic mix: 70% votes, 15% transfers, 8% tokens, 5% DeFi, 2% NFT\n";
  std::cout << "Slot time: 400ms\n";

  // Test 1: Normal Solana load (~2000 entries/block)
  test_sustained_load(2000, 5);

  // Test 2: Heavy load (~5000 entries/block)
  test_sustained_load(5000, 5);

  // Test 3: Extreme load (~10000 entries/block)
  test_sustained_load(10000, 3);

  // Test 4: Burst throughput
  test_burst_load();

  // Test 5: Memory pressure
  test_memory_pressure();

  std::cout << "================================================================\n";

  return 0;
}

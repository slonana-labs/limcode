/**
 * @file main.cpp
 * @brief Benchmark comparing limcode vs wincode vs bincode
 */

#include <limcode/limcode.h>
#include <limcode/wincode.h>
#include <limcode/bincode.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>

using namespace std::chrono;
using namespace limcode;

// ==================== Test Data Generator ====================

class DataGenerator {
public:
  explicit DataGenerator(uint32_t seed = 42) : rng_(seed) {}

  Entry generate_vote_entry() {
    Entry e;
    e.num_hashes = dist_(rng_) % 1000;
    e.hash = random_hash();

    VersionedTransaction tx;
    tx.signatures.push_back(random_signature());

    LegacyMessage msg;
    msg.header = {1, 0, 5};
    for (int i = 0; i < 6; ++i) {
      msg.account_keys.push_back(random_hash());
    }
    msg.recent_blockhash = random_hash();

    CompiledInstruction instr;
    instr.program_id_index = 5;
    instr.accounts = {0, 1, 2, 3, 4};
    instr.data = random_bytes(32);
    msg.instructions.push_back(std::move(instr));

    tx.message.set_legacy(std::move(msg));
    e.transactions.push_back(std::move(tx));
    return e;
  }

  Entry generate_transfer_entry() {
    Entry e;
    e.num_hashes = dist_(rng_) % 500;
    e.hash = random_hash();

    VersionedTransaction tx;
    tx.signatures.push_back(random_signature());

    LegacyMessage msg;
    msg.header = {1, 0, 1};
    msg.account_keys.push_back(random_hash());
    msg.account_keys.push_back(random_hash());
    msg.account_keys.push_back(random_hash());
    msg.recent_blockhash = random_hash();

    CompiledInstruction instr;
    instr.program_id_index = 2;
    instr.accounts = {0, 1};
    instr.data = random_bytes(12);
    msg.instructions.push_back(std::move(instr));

    tx.message.set_legacy(std::move(msg));
    e.transactions.push_back(std::move(tx));
    return e;
  }

  Entry generate_defi_entry() {
    Entry e;
    e.num_hashes = dist_(rng_) % 200;
    e.hash = random_hash();

    VersionedTransaction tx;
    tx.signatures.push_back(random_signature());
    tx.signatures.push_back(random_signature());

    V0Message msg;
    msg.header = {2, 1, 4};
    for (int i = 0; i < 8; ++i) {
      msg.account_keys.push_back(random_hash());
    }
    msg.recent_blockhash = random_hash();

    for (int i = 0; i < 3; ++i) {
      CompiledInstruction instr;
      instr.program_id_index = static_cast<uint8_t>(5 + i);
      instr.accounts = {0, 1, 2, 3};
      instr.data = random_bytes(64);
      msg.instructions.push_back(std::move(instr));
    }

    AddressTableLookup atl;
    atl.account_key = random_hash();
    atl.writable_indexes = {0, 1, 2};
    atl.readonly_indexes = {3, 4};
    msg.address_table_lookups.push_back(std::move(atl));

    tx.message.set_v0(std::move(msg));
    e.transactions.push_back(std::move(tx));
    return e;
  }

  // Generate realistic block: 70% votes, 20% transfers, 10% DeFi
  std::vector<Entry> generate_block(size_t num_entries) {
    std::vector<Entry> entries;
    entries.reserve(num_entries);

    std::uniform_int_distribution<int> type_dist(1, 100);

    for (size_t i = 0; i < num_entries; ++i) {
      int roll = type_dist(rng_);
      if (roll <= 70) {
        entries.push_back(generate_vote_entry());
      } else if (roll <= 90) {
        entries.push_back(generate_transfer_entry());
      } else {
        entries.push_back(generate_defi_entry());
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

// ==================== Benchmark ====================

struct BenchResult {
  double blocks_per_sec;
  double throughput_gbps;
  size_t bytes_per_block;
};

BenchResult benchmark_limcode(const std::vector<Entry>& entries, int iterations) {
  // Warmup
  for (int i = 0; i < 10; ++i) {
    auto span = limcode::serialize(entries);
    (void)span.size();
  }

  size_t total_bytes = 0;
  auto start = high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i) {
    auto span = limcode::serialize(entries);
    total_bytes += span.size();
  }
  auto end = high_resolution_clock::now();

  double ms = duration_cast<microseconds>(end - start).count() / 1000.0;
  double blocks_per_sec = (iterations / ms) * 1000.0;
  double throughput_gbps = (total_bytes * 8.0) / (ms * 1e6);
  size_t bytes_per_block = total_bytes / iterations;

  return {blocks_per_sec, throughput_gbps, bytes_per_block};
}

BenchResult benchmark_wincode(const std::vector<Entry>& entries, int iterations) {
  // Warmup
  for (int i = 0; i < 10; ++i) {
    auto bytes = wincode::serialize(entries);
    (void)bytes.size();
  }

  size_t total_bytes = 0;
  auto start = high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i) {
    auto bytes = wincode::serialize(entries);
    total_bytes += bytes.size();
  }
  auto end = high_resolution_clock::now();

  double ms = duration_cast<microseconds>(end - start).count() / 1000.0;
  double blocks_per_sec = (iterations / ms) * 1000.0;
  double throughput_gbps = (total_bytes * 8.0) / (ms * 1e6);
  size_t bytes_per_block = total_bytes / iterations;

  return {blocks_per_sec, throughput_gbps, bytes_per_block};
}

BenchResult benchmark_bincode(const std::vector<Entry>& entries, int iterations) {
  // Warmup
  for (int i = 0; i < 10; ++i) {
    auto bytes = bincode::serialize(entries);
    (void)bytes.size();
  }

  size_t total_bytes = 0;
  auto start = high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i) {
    auto bytes = bincode::serialize(entries);
    total_bytes += bytes.size();
  }
  auto end = high_resolution_clock::now();

  double ms = duration_cast<microseconds>(end - start).count() / 1000.0;
  double blocks_per_sec = (iterations / ms) * 1000.0;
  double throughput_gbps = (total_bytes * 8.0) / (ms * 1e6);
  size_t bytes_per_block = total_bytes / iterations;

  return {blocks_per_sec, throughput_gbps, bytes_per_block};
}

void print_results(const char* name, const BenchResult& r) {
  std::cout << "  " << std::setw(12) << std::left << name << ": "
            << std::setw(12) << std::right << std::fixed << std::setprecision(0)
            << r.blocks_per_sec << " blocks/s, "
            << std::setw(6) << std::setprecision(1) << r.throughput_gbps << " Gbps, "
            << std::setw(6) << (r.bytes_per_block / 1024.0) << " KB/block\n";
}

int main() {
  std::cout << "\n";
  std::cout << "================================================================\n";
  std::cout << "         LIMCODE vs WINCODE vs BINCODE Benchmark\n";
  std::cout << "================================================================\n";
  std::cout << "Transaction mix: 70% votes, 20% transfers, 10% DeFi\n\n";

  DataGenerator gen;

  std::cout << "| Block Size | Limcode | Wincode | Bincode | Limcode vs Wincode | Limcode vs Bincode |\n";
  std::cout << "|------------|---------|---------|---------|--------------------|--------------------|" << std::endl;

  for (size_t block_size : {100, 500, 1000, 2000, 5000}) {
    auto entries = gen.generate_block(block_size);

    int iterations = (block_size <= 500) ? 500 : 200;

    auto lim = benchmark_limcode(entries, iterations);
    auto win = benchmark_wincode(entries, iterations);
    auto bin = benchmark_bincode(entries, iterations);

    double lim_vs_win = lim.blocks_per_sec / win.blocks_per_sec;
    double lim_vs_bin = lim.blocks_per_sec / bin.blocks_per_sec;

    std::cout << "| " << std::setw(10) << block_size << " | "
              << std::setw(7) << std::fixed << std::setprecision(0) << lim.blocks_per_sec << " | "
              << std::setw(7) << win.blocks_per_sec << " | "
              << std::setw(7) << bin.blocks_per_sec << " | "
              << std::setw(18) << std::setprecision(2) << lim_vs_win << "x | "
              << std::setw(18) << lim_vs_bin << "x |" << std::endl;
  }

  std::cout << "\n";
  std::cout << "================================================================\n";
  std::cout << "                    Detailed Results\n";
  std::cout << "================================================================\n\n";

  for (size_t block_size : {100, 500, 1000, 2000}) {
    std::cout << "--- Block Size: " << block_size << " entries ---\n";

    auto entries = gen.generate_block(block_size);
    int iterations = (block_size <= 500) ? 500 : 200;

    auto lim = benchmark_limcode(entries, iterations);
    auto win = benchmark_wincode(entries, iterations);
    auto bin = benchmark_bincode(entries, iterations);

    print_results("Limcode", lim);
    print_results("Wincode", win);
    print_results("Bincode", bin);

    double lim_vs_win = lim.blocks_per_sec / win.blocks_per_sec;
    double lim_vs_bin = lim.blocks_per_sec / bin.blocks_per_sec;

    std::cout << "  -> Limcode is " << std::fixed << std::setprecision(2)
              << lim_vs_win << "x faster than Wincode, "
              << lim_vs_bin << "x faster than Bincode\n\n";
  }

  std::cout << "================================================================\n";

  return 0;
}

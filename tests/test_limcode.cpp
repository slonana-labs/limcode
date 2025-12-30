/**
 * @file test_limcode.cpp
 * @brief Basic tests for limcode serialization
 */

#include <limcode/limcode.h>
#include <limcode/wincode.h>
#include <limcode/bincode.h>

#include <cassert>
#include <iostream>

using namespace limcode;

void test_entry_serialization() {
  Entry e;
  e.num_hashes = 12345;
  for (auto& b : e.hash) b = 0xAB;

  VersionedTransaction tx;
  std::array<uint8_t, 64> sig;
  for (auto& b : sig) b = 0xCD;
  tx.signatures.push_back(sig);

  LegacyMessage msg;
  msg.header = {1, 0, 1};
  std::array<uint8_t, 32> key1, key2, key3;
  for (auto& b : key1) b = 0x11;
  for (auto& b : key2) b = 0x22;
  for (auto& b : key3) b = 0x33;
  msg.account_keys = {key1, key2, key3};
  for (auto& b : msg.recent_blockhash) b = 0xEE;

  CompiledInstruction instr;
  instr.program_id_index = 2;
  instr.accounts = {0, 1};
  instr.data = {1, 2, 3, 4, 5, 6, 7, 8};
  msg.instructions.push_back(std::move(instr));

  tx.message.set_legacy(std::move(msg));
  e.transactions.push_back(std::move(tx));

  // Serialize with limcode
  auto lim_span = limcode::serialize_entry(e);
  std::vector<uint8_t> lim_bytes(lim_span.begin(), lim_span.end());

  // Serialize with wincode
  auto win_bytes = wincode::serialize_entry(e);

  // They should be identical (same wire format)
  assert(lim_bytes == win_bytes && "Limcode and Wincode should produce identical output");

  std::cout << "  Entry serialization: PASS (" << lim_bytes.size() << " bytes)\n";
}

void test_batch_serialization() {
  std::vector<Entry> entries;

  for (int i = 0; i < 10; ++i) {
    Entry e;
    e.num_hashes = static_cast<uint64_t>(i * 100);
    for (auto& b : e.hash) b = static_cast<uint8_t>(i);
    entries.push_back(std::move(e));
  }

  auto lim_span = limcode::serialize(entries);
  std::vector<uint8_t> lim_bytes(lim_span.begin(), lim_span.end());

  auto win_bytes = wincode::serialize(entries);

  assert(lim_bytes == win_bytes && "Batch serialization should match");

  std::cout << "  Batch serialization: PASS (" << lim_bytes.size() << " bytes)\n";
}

void test_v0_message() {
  Entry e;
  e.num_hashes = 999;
  for (auto& b : e.hash) b = 0xFF;

  VersionedTransaction tx;
  std::array<uint8_t, 64> sig;
  for (auto& b : sig) b = 0xAA;
  tx.signatures.push_back(sig);

  V0Message msg;
  msg.header = {1, 0, 2};
  std::array<uint8_t, 32> key;
  for (auto& b : key) b = 0xBB;
  msg.account_keys = {key, key, key};
  for (auto& b : msg.recent_blockhash) b = 0xCC;

  CompiledInstruction instr;
  instr.program_id_index = 2;
  instr.accounts = {0, 1};
  instr.data = {0xDE, 0xAD, 0xBE, 0xEF};
  msg.instructions.push_back(std::move(instr));

  AddressTableLookup atl;
  for (auto& b : atl.account_key) b = 0xDD;
  atl.writable_indexes = {0, 1};
  atl.readonly_indexes = {2};
  msg.address_table_lookups.push_back(std::move(atl));

  tx.message.set_v0(std::move(msg));
  e.transactions.push_back(std::move(tx));

  auto lim_span = limcode::serialize_entry(e);
  std::vector<uint8_t> lim_bytes(lim_span.begin(), lim_span.end());

  auto win_bytes = wincode::serialize_entry(e);

  assert(lim_bytes == win_bytes && "V0 message serialization should match");

  std::cout << "  V0 message serialization: PASS (" << lim_bytes.size() << " bytes)\n";
}

void test_bincode_different_size() {
  Entry e;
  e.num_hashes = 42;
  for (auto& b : e.hash) b = 0x00;

  VersionedTransaction tx;
  std::array<uint8_t, 64> sig{};
  tx.signatures.push_back(sig);

  LegacyMessage msg;
  msg.header = {1, 0, 0};
  std::array<uint8_t, 32> key{};
  msg.account_keys = {key};
  msg.recent_blockhash = {};

  tx.message.set_legacy(std::move(msg));
  e.transactions.push_back(std::move(tx));

  auto win_bytes = wincode::serialize_entry(e);
  auto bin_bytes = bincode::serialize_entry(e);

  // Bincode should be larger due to u64 lengths vs ShortVec
  assert(bin_bytes.size() > win_bytes.size() && "Bincode should be larger than Wincode");

  std::cout << "  Bincode vs Wincode size: PASS (bincode=" << bin_bytes.size()
            << ", wincode=" << win_bytes.size() << ")\n";
}

int main() {
  std::cout << "\n";
  std::cout << "================================================================\n";
  std::cout << "                    Limcode Tests\n";
  std::cout << "================================================================\n\n";

  test_entry_serialization();
  test_batch_serialization();
  test_v0_message();
  test_bincode_different_size();

  std::cout << "\nAll tests passed!\n";
  std::cout << "================================================================\n";

  return 0;
}

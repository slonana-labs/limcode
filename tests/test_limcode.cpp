/**
 * @file test_limcode.cpp
 * @brief Basic tests for limcode serialization
 */

#include <limcode/limcode.h>

#include <cassert>
#include <iostream>

using namespace limcode;

void test_entry_serialization() {
  Entry e;
  e.num_hashes = 12345;
  for (auto &b : e.hash)
    b = 0xAB;

  VersionedTransaction tx;
  std::array<uint8_t, 64> sig;
  for (auto &b : sig)
    b = 0xCD;
  tx.signatures.push_back(sig);

  LegacyMessage msg;
  msg.header = {1, 0, 1};
  std::array<uint8_t, 32> key1, key2, key3;
  for (auto &b : key1)
    b = 0x11;
  for (auto &b : key2)
    b = 0x22;
  for (auto &b : key3)
    b = 0x33;
  msg.account_keys = {key1, key2, key3};
  for (auto &b : msg.recent_blockhash)
    b = 0xEE;

  CompiledInstruction instr;
  instr.program_id_index = 2;
  instr.accounts = {0, 1};
  instr.data = {1, 2, 3, 4, 5, 6, 7, 8};
  msg.instructions.push_back(std::move(instr));

  tx.message.set_legacy(std::move(msg));
  e.transactions.push_back(std::move(tx));

  // Serialize with limcode
  auto bytes = limcode::serialize_entry(e);

  // Basic sanity checks
  assert(bytes.size() > 0 && "Serialized data should not be empty");
  assert(bytes.size() < 1000 && "Entry should be reasonably sized");

  std::cout << "  Entry serialization: PASS (" << bytes.size() << " bytes)\n";
}

void test_batch_serialization() {
  std::vector<Entry> entries;

  for (int i = 0; i < 10; ++i) {
    Entry e;
    e.num_hashes = static_cast<uint64_t>(i * 100);
    for (auto &b : e.hash)
      b = static_cast<uint8_t>(i);
    entries.push_back(std::move(e));
  }

  auto bytes = limcode::serialize(entries);

  // Should start with entry count (u64 = 10)
  assert(bytes.size() > 8 && "Should include length prefix");

  std::cout << "  Batch serialization: PASS (" << bytes.size() << " bytes)\n";
}

void test_v0_message() {
  Entry e;
  e.num_hashes = 999;
  for (auto &b : e.hash)
    b = 0xFF;

  VersionedTransaction tx;
  std::array<uint8_t, 64> sig;
  for (auto &b : sig)
    b = 0xAA;
  tx.signatures.push_back(sig);

  V0Message msg;
  msg.header = {1, 0, 2};
  std::array<uint8_t, 32> key;
  for (auto &b : key)
    b = 0xBB;
  msg.account_keys = {key, key, key};
  for (auto &b : msg.recent_blockhash)
    b = 0xCC;

  CompiledInstruction instr;
  instr.program_id_index = 2;
  instr.accounts = {0, 1};
  instr.data = {0xDE, 0xAD, 0xBE, 0xEF};
  msg.instructions.push_back(std::move(instr));

  AddressTableLookup atl;
  for (auto &b : atl.account_key)
    b = 0xDD;
  atl.writable_indexes = {0, 1};
  atl.readonly_indexes = {2};
  msg.address_table_lookups.push_back(std::move(atl));

  tx.message.set_v0(std::move(msg));
  e.transactions.push_back(std::move(tx));

  auto bytes = limcode::serialize_entry(e);

  // V0 messages should have version prefix
  assert(bytes.size() > 0 && "Serialized data should not be empty");

  std::cout << "  V0 message serialization: PASS (" << bytes.size()
            << " bytes)\n";
}

void test_round_trip() {
  // Create an entry with complex data
  Entry original;
  original.num_hashes = 42;
  for (auto &b : original.hash)
    b = 0xAB;

  VersionedTransaction tx;
  std::array<uint8_t, 64> sig{};
  for (auto &b : sig)
    b = 0xCD;
  tx.signatures.push_back(sig);

  LegacyMessage msg;
  msg.header = {1, 0, 0};
  std::array<uint8_t, 32> key{};
  for (auto &b : key)
    b = 0x99;
  msg.account_keys = {key};
  msg.recent_blockhash = {};

  tx.message.set_legacy(std::move(msg));
  original.transactions.push_back(std::move(tx));

  // Serialize
  auto bytes = limcode::serialize_entry(original);

  // Deserialize
  auto decoded = limcode::deserialize_entry(bytes);

  // Verify round-trip
  assert(decoded.num_hashes == original.num_hashes &&
         "num_hashes should match");
  assert(decoded.hash == original.hash && "hash should match");
  assert(decoded.transactions.size() == original.transactions.size() &&
         "transaction count should match");

  std::cout << "  Round-trip serialization: PASS\n";
}

int main() {
  std::cout << "\n";
  std::cout
      << "================================================================\n";
  std::cout << "                    Limcode Tests\n";
  std::cout
      << "================================================================\n\n";

  test_entry_serialization();
  test_batch_serialization();
  test_v0_message();
  test_round_trip();

  std::cout << "\nAll tests passed!\n";
  std::cout
      << "================================================================\n";

  return 0;
}

#pragma once

/**
 * @file types.h
 * @brief Common types for Solana serialization
 */

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <vector>

namespace limcode {

// ==================== Constants ====================

constexpr size_t HASH_BYTES = 32;
constexpr size_t PUBKEY_BYTES = 32;
constexpr size_t SIGNATURE_BYTES = 64;
constexpr uint8_t VERSION_PREFIX_MASK = 0x80;

// ==================== Core Types ====================

struct MessageHeader {
  uint8_t num_required_signatures{0};
  uint8_t num_readonly_signed_accounts{0};
  uint8_t num_readonly_unsigned_accounts{0};
};

struct CompiledInstruction {
  uint8_t program_id_index{0};
  std::vector<uint8_t> accounts;
  std::vector<uint8_t> data;
};

struct AddressTableLookup {
  std::array<uint8_t, PUBKEY_BYTES> account_key{};
  std::vector<uint8_t> writable_indexes;
  std::vector<uint8_t> readonly_indexes;
};

struct LegacyMessage {
  MessageHeader header;
  std::vector<std::array<uint8_t, PUBKEY_BYTES>> account_keys;
  std::array<uint8_t, HASH_BYTES> recent_blockhash{};
  std::vector<CompiledInstruction> instructions;
};

struct V0Message {
  MessageHeader header;
  std::vector<std::array<uint8_t, PUBKEY_BYTES>> account_keys;
  std::array<uint8_t, HASH_BYTES> recent_blockhash{};
  std::vector<CompiledInstruction> instructions;
  std::vector<AddressTableLookup> address_table_lookups;
};

class VersionedMessage {
public:
  VersionedMessage() : is_v0_(false) {}

  void set_legacy(LegacyMessage msg) {
    legacy_ = std::move(msg);
    is_v0_ = false;
  }

  void set_v0(V0Message msg) {
    v0_ = std::move(msg);
    is_v0_ = true;
  }

  [[nodiscard]] bool is_v0() const noexcept { return is_v0_; }
  [[nodiscard]] const LegacyMessage& as_legacy() const { return legacy_; }
  [[nodiscard]] const V0Message& as_v0() const { return v0_; }
  [[nodiscard]] LegacyMessage& as_legacy() { return legacy_; }
  [[nodiscard]] V0Message& as_v0() { return v0_; }

private:
  bool is_v0_;
  LegacyMessage legacy_;
  V0Message v0_;
};

struct VersionedTransaction {
  std::vector<std::array<uint8_t, SIGNATURE_BYTES>> signatures;
  VersionedMessage message;
};

struct Entry {
  uint64_t num_hashes{0};
  std::array<uint8_t, HASH_BYTES> hash{};
  std::vector<VersionedTransaction> transactions;
};

// ==================== Exceptions ====================

class SerializationError : public std::runtime_error {
public:
  explicit SerializationError(const char* msg) : std::runtime_error(msg) {}
};

}  // namespace limcode

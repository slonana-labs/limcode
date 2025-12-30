#pragma once

/**
 * @file wincode.h
 * @brief Wincode - Solana's serialization format (ShortVec for lengths)
 *
 * Wincode is Solana's variant of bincode that uses ShortVec (varint 1-3 bytes)
 * for vector lengths instead of u64. This saves bytes for small vectors.
 *
 * This is the baseline implementation for comparison with limcode.
 */

#include "types.h"

namespace wincode {

using namespace limcode;

class Serializer {
public:
  Serializer() { buffer_.reserve(4096); }

  void reset() { buffer_.clear(); }

  [[nodiscard]] const std::vector<uint8_t>& data() const { return buffer_; }
  [[nodiscard]] std::vector<uint8_t> finish() { return std::move(buffer_); }

  void write_u8(uint8_t v) { buffer_.push_back(v); }

  void write_u64(uint64_t v) {
    size_t pos = buffer_.size();
    buffer_.resize(pos + 8);
    std::memcpy(buffer_.data() + pos, &v, 8);
  }

  void write_shortvec(uint16_t len) {
    if (len < 0x80) {
      buffer_.push_back(static_cast<uint8_t>(len));
    } else if (len < 0x4000) {
      buffer_.push_back(static_cast<uint8_t>((len & 0x7F) | 0x80));
      buffer_.push_back(static_cast<uint8_t>(len >> 7));
    } else {
      buffer_.push_back(static_cast<uint8_t>((len & 0x7F) | 0x80));
      buffer_.push_back(static_cast<uint8_t>(((len >> 7) & 0x7F) | 0x80));
      buffer_.push_back(static_cast<uint8_t>(len >> 14));
    }
  }

  void write_bytes(const uint8_t* src, size_t len) {
    size_t pos = buffer_.size();
    buffer_.resize(pos + len);
    std::memcpy(buffer_.data() + pos, src, len);
  }

  void write_hash(const std::array<uint8_t, 32>& h) {
    write_bytes(h.data(), 32);
  }

  void write_signature(const std::array<uint8_t, 64>& s) {
    write_bytes(s.data(), 64);
  }

  void write_pubkey(const std::array<uint8_t, 32>& p) {
    write_bytes(p.data(), 32);
  }

  void write_instruction(const CompiledInstruction& instr) {
    write_u8(instr.program_id_index);
    write_shortvec(static_cast<uint16_t>(instr.accounts.size()));
    write_bytes(instr.accounts.data(), instr.accounts.size());
    write_shortvec(static_cast<uint16_t>(instr.data.size()));
    write_bytes(instr.data.data(), instr.data.size());
  }

  void write_legacy_message(const LegacyMessage& msg) {
    write_u8(msg.header.num_required_signatures);
    write_u8(msg.header.num_readonly_signed_accounts);
    write_u8(msg.header.num_readonly_unsigned_accounts);

    write_shortvec(static_cast<uint16_t>(msg.account_keys.size()));
    for (const auto& key : msg.account_keys) {
      write_pubkey(key);
    }

    write_hash(msg.recent_blockhash);

    write_shortvec(static_cast<uint16_t>(msg.instructions.size()));
    for (const auto& instr : msg.instructions) {
      write_instruction(instr);
    }
  }

  void write_v0_message(const V0Message& msg) {
    write_u8(msg.header.num_required_signatures);
    write_u8(msg.header.num_readonly_signed_accounts);
    write_u8(msg.header.num_readonly_unsigned_accounts);

    write_shortvec(static_cast<uint16_t>(msg.account_keys.size()));
    for (const auto& key : msg.account_keys) {
      write_pubkey(key);
    }

    write_hash(msg.recent_blockhash);

    write_shortvec(static_cast<uint16_t>(msg.instructions.size()));
    for (const auto& instr : msg.instructions) {
      write_instruction(instr);
    }

    write_shortvec(static_cast<uint16_t>(msg.address_table_lookups.size()));
    for (const auto& atl : msg.address_table_lookups) {
      write_pubkey(atl.account_key);
      write_shortvec(static_cast<uint16_t>(atl.writable_indexes.size()));
      write_bytes(atl.writable_indexes.data(), atl.writable_indexes.size());
      write_shortvec(static_cast<uint16_t>(atl.readonly_indexes.size()));
      write_bytes(atl.readonly_indexes.data(), atl.readonly_indexes.size());
    }
  }

  void write_message(const VersionedMessage& msg) {
    if (msg.is_v0()) {
      write_u8(VERSION_PREFIX_MASK);
      write_v0_message(msg.as_v0());
    } else {
      write_legacy_message(msg.as_legacy());
    }
  }

  void write_transaction(const VersionedTransaction& tx) {
    write_shortvec(static_cast<uint16_t>(tx.signatures.size()));
    for (const auto& sig : tx.signatures) {
      write_signature(sig);
    }
    write_message(tx.message);
  }

  void write_entry(const Entry& entry) {
    write_u64(entry.num_hashes);
    write_hash(entry.hash);
    write_shortvec(static_cast<uint16_t>(entry.transactions.size()));
    for (const auto& tx : entry.transactions) {
      write_transaction(tx);
    }
  }

private:
  std::vector<uint8_t> buffer_;
};

// ==================== Public API ====================

[[nodiscard]] inline std::vector<uint8_t> serialize_entry(const Entry& entry) {
  Serializer s;
  s.write_entry(entry);
  return s.finish();
}

[[nodiscard]] inline std::vector<uint8_t> serialize(const std::vector<Entry>& entries) {
  Serializer s;
  s.write_u64(entries.size());
  for (const auto& entry : entries) {
    s.write_entry(entry);
  }
  return s.finish();
}

[[nodiscard]] inline std::vector<uint8_t> serialize_transaction(const VersionedTransaction& tx) {
  Serializer s;
  s.write_transaction(tx);
  return s.finish();
}

}  // namespace wincode

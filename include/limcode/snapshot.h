#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <array>

namespace limcode {
namespace snapshot {

/// Solana account from snapshot AppendVec format
struct SnapshotAccount {
    uint64_t write_version;           // Obsolete but present in format
    std::array<uint8_t, 32> pubkey;
    uint64_t lamports;
    uint64_t rent_epoch;
    std::array<uint8_t, 32> owner;
    bool executable;
    std::array<uint8_t, 32> hash;     // Account state hash
    std::vector<uint8_t> data;        // Variable length account data

    SnapshotAccount() : write_version(0), lamports(0), rent_epoch(0), executable(false) {
        pubkey.fill(0);
        owner.fill(0);
        hash.fill(0);
    }
};

/// AppendVec account header (136 bytes)
#pragma pack(push, 1)
struct AppendVecHeader {
    uint64_t write_version;          // 8 bytes - offset 0x00
    uint64_t data_len;              // 8 bytes - offset 0x08
    uint8_t pubkey[32];             // 32 bytes - offset 0x10
    uint64_t lamports;              // 8 bytes - offset 0x30
    uint64_t rent_epoch;            // 8 bytes - offset 0x38
    uint8_t owner[32];              // 32 bytes - offset 0x40
    uint8_t executable;             // 1 byte - offset 0x60
    uint8_t padding[7];             // 7 bytes - offset 0x61
    uint8_t hash[32];               // 32 bytes - offset 0x68
    // Total: 136 bytes (0x88)
};
#pragma pack(pop)

static_assert(sizeof(AppendVecHeader) == 136, "AppendVecHeader must be 136 bytes");

/// Parse accounts from AppendVec file data
///
/// @param data Raw AppendVec file data
/// @param size Size of data in bytes
/// @param accounts Output vector to store parsed accounts
/// @return Number of accounts parsed
size_t parse_appendvec(const uint8_t* data, size_t size, std::vector<SnapshotAccount>& accounts);

/// Stream accounts from AppendVec file data using callback
///
/// Memory-efficient: processes one account at a time
///
/// @param data Raw AppendVec file data
/// @param size Size of data in bytes
/// @param callback Function called for each account (return false to stop)
/// @return Number of accounts processed
size_t stream_appendvec(const uint8_t* data, size_t size,
                       std::function<bool(const SnapshotAccount&)> callback);

/// Parse Solana snapshot archive (.tar.zst)
///
/// WARNING: Loads all accounts into memory. Use stream_snapshot for large snapshots.
///
/// Real Solana snapshot structure:
/// - version (contains "1.2.0")
/// - status_cache
/// - snapshots/SLOT/SLOT (manifest file - bincode serialized)
/// - accounts/SLOT.ID (AppendVec files - 136-byte headers)
///
/// @param snapshot_path Path to .tar.zst snapshot archive
/// @param accounts Output vector to store all accounts
/// @return true on success, false on error
bool parse_snapshot(const std::string& snapshot_path, std::vector<SnapshotAccount>& accounts);

/// Stream accounts from Solana snapshot archive (memory-efficient for 1.5B+ accounts)
///
/// @param snapshot_path Path to .tar.zst snapshot archive
/// @param callback Function called for each account (return false to stop)
/// @return Number of accounts processed, or -1 on error
int64_t stream_snapshot(const std::string& snapshot_path,
                       std::function<bool(const SnapshotAccount&)> callback);

/// Statistics from snapshot parsing
struct SnapshotStats {
    uint64_t total_accounts = 0;
    uint64_t total_lamports = 0;
    uint64_t total_data_bytes = 0;
    uint64_t executable_accounts = 0;
    size_t max_data_size = 0;
    double parse_time_seconds = 0.0;

    double accounts_per_second() const {
        return parse_time_seconds > 0 ? total_accounts / parse_time_seconds : 0;
    }

    uint64_t data_accounts() const {
        return total_accounts - executable_accounts;
    }

    double total_sol() const {
        return total_lamports / 1e9;
    }

    double total_data_mb() const {
        return total_data_bytes / 1e6;
    }
};

/// Parse snapshot and collect statistics
///
/// @param snapshot_path Path to .tar.zst snapshot archive
/// @param stats Output statistics
/// @return Number of accounts processed, or -1 on error
int64_t parse_snapshot_stats(const std::string& snapshot_path, SnapshotStats& stats);

} // namespace snapshot
} // namespace limcode

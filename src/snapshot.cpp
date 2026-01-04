#include "limcode/snapshot.h"
#include <archive.h>
#include <archive_entry.h>
#include <cstring>
#include <chrono>
#include <iostream>

namespace limcode {
namespace snapshot {

size_t parse_appendvec(const uint8_t* data, size_t size, std::vector<SnapshotAccount>& accounts) {
    size_t count = 0;
    size_t offset = 0;
    constexpr size_t HEADER_SIZE = sizeof(AppendVecHeader);

    while (offset + HEADER_SIZE <= size) {
        const auto* header = reinterpret_cast<const AppendVecHeader*>(data + offset);

        // Validate data_len
        if (offset + HEADER_SIZE + header->data_len > size) {
            break; // Incomplete account
        }

        SnapshotAccount account;
        account.write_version = header->write_version;
        account.lamports = header->lamports;
        account.rent_epoch = header->rent_epoch;
        account.executable = header->executable != 0;

        std::memcpy(account.pubkey.data(), header->pubkey, 32);
        std::memcpy(account.owner.data(), header->owner, 32);
        std::memcpy(account.hash.data(), header->hash, 32);

        offset += HEADER_SIZE;

        // Read variable-length account data
        if (header->data_len > 0) {
            account.data.resize(header->data_len);
            std::memcpy(account.data.data(), data + offset, header->data_len);
            offset += header->data_len;
        }

        // 8-byte alignment padding
        size_t padding = (8 - (offset % 8)) % 8;
        offset += padding;

        accounts.push_back(std::move(account));
        count++;
    }

    return count;
}

size_t stream_appendvec(const uint8_t* data, size_t size,
                       std::function<bool(const SnapshotAccount&)> callback) {
    size_t count = 0;
    size_t offset = 0;
    constexpr size_t HEADER_SIZE = sizeof(AppendVecHeader);

    while (offset + HEADER_SIZE <= size) {
        const auto* header = reinterpret_cast<const AppendVecHeader*>(data + offset);

        // Validate data_len
        if (offset + HEADER_SIZE + header->data_len > size) {
            break; // Incomplete account
        }

        SnapshotAccount account;
        account.write_version = header->write_version;
        account.lamports = header->lamports;
        account.rent_epoch = header->rent_epoch;
        account.executable = header->executable != 0;

        std::memcpy(account.pubkey.data(), header->pubkey, 32);
        std::memcpy(account.owner.data(), header->owner, 32);
        std::memcpy(account.hash.data(), header->hash, 32);

        offset += HEADER_SIZE;

        // Read variable-length account data
        if (header->data_len > 0) {
            account.data.resize(header->data_len);
            std::memcpy(account.data.data(), data + offset, header->data_len);
            offset += header->data_len;
        }

        // 8-byte alignment padding
        size_t padding = (8 - (offset % 8)) % 8;
        offset += padding;

        // Call callback with account
        if (!callback(account)) {
            break; // User requested stop
        }

        count++;
    }

    return count;
}

bool parse_snapshot(const std::string& snapshot_path, std::vector<SnapshotAccount>& accounts) {
    struct archive* a = archive_read_new();
    archive_read_support_filter_zstd(a);
    archive_read_support_format_tar(a);

    if (archive_read_open_filename(a, snapshot_path.c_str(), 10240) != ARCHIVE_OK) {
        archive_read_free(a);
        return false;
    }

    struct archive_entry* entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);

        // Look for AppendVec files in accounts/ directory
        if (std::strncmp(pathname, "accounts/", 9) == 0) {
            // Read entire file into buffer
            size_t size = archive_entry_size(entry);
            std::vector<uint8_t> buffer(size);

            ssize_t bytes_read = archive_read_data(a, buffer.data(), size);
            if (bytes_read > 0) {
                parse_appendvec(buffer.data(), bytes_read, accounts);
            }
        }

        archive_read_data_skip(a);
    }

    archive_read_free(a);
    return true;
}

int64_t stream_snapshot(const std::string& snapshot_path,
                       std::function<bool(const SnapshotAccount&)> callback) {
    struct archive* a = archive_read_new();
    archive_read_support_filter_zstd(a);
    archive_read_support_format_tar(a);

    if (archive_read_open_filename(a, snapshot_path.c_str(), 10240) != ARCHIVE_OK) {
        archive_read_free(a);
        return -1;
    }

    int64_t total_accounts = 0;
    struct archive_entry* entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);

        // Look for AppendVec files in accounts/ directory
        if (std::strncmp(pathname, "accounts/", 9) == 0) {
            // Read entire AppendVec file
            size_t size = archive_entry_size(entry);
            std::vector<uint8_t> buffer(size);

            ssize_t bytes_read = archive_read_data(a, buffer.data(), size);
            if (bytes_read > 0) {
                // Stream accounts from this AppendVec file
                size_t count = stream_appendvec(buffer.data(), bytes_read, callback);
                total_accounts += count;
            }
        }

        archive_read_data_skip(a);
    }

    archive_read_free(a);
    return total_accounts;
}

// OPTIMIZED: Direct parsing without callback overhead
int64_t parse_snapshot_stats(const std::string& snapshot_path, SnapshotStats& stats) {
    auto start = std::chrono::high_resolution_clock::now();

    struct archive* a = archive_read_new();
    archive_read_support_filter_zstd(a);
    archive_read_support_format_tar(a);

    if (archive_read_open_filename(a, snapshot_path.c_str(), 10240) != ARCHIVE_OK) {
        archive_read_free(a);
        return -1;
    }

    int64_t total_accounts = 0;
    struct archive_entry* entry;
    constexpr size_t HEADER_SIZE = sizeof(AppendVecHeader);

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);

        // Look for AppendVec files in accounts/ directory
        if (std::strncmp(pathname, "accounts/", 9) == 0) {
            // Read entire AppendVec file
            size_t size = archive_entry_size(entry);
            std::vector<uint8_t> buffer(size);

            ssize_t bytes_read = archive_read_data(a, buffer.data(), size);
            if (bytes_read > 0) {
                // Parse accounts directly (no callback overhead)
                const uint8_t* data = buffer.data();
                size_t offset = 0;

                while (offset + HEADER_SIZE <= static_cast<size_t>(bytes_read)) {
                    const auto* header = reinterpret_cast<const AppendVecHeader*>(data + offset);

                    if (offset + HEADER_SIZE + header->data_len > static_cast<size_t>(bytes_read)) {
                        break;
                    }

                    // Update stats directly (FAST PATH)
                    stats.total_accounts++;
                    stats.total_lamports += header->lamports;
                    stats.total_data_bytes += header->data_len;

                    if (header->executable) {
                        stats.executable_accounts++;
                    }

                    if (header->data_len > stats.max_data_size) {
                        stats.max_data_size = header->data_len;
                    }

                    offset += HEADER_SIZE + header->data_len;

                    // 8-byte alignment
                    size_t padding = (8 - (offset % 8)) % 8;
                    offset += padding;

                    total_accounts++;
                }
            }
        }

        archive_read_data_skip(a);
    }

    archive_read_free(a);

    auto end = std::chrono::high_resolution_clock::now();
    stats.parse_time_seconds = std::chrono::duration<double>(end - start).count();

    return total_accounts;
}

} // namespace snapshot
} // namespace limcode

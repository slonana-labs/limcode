// ULTRA-FAST: Direct zstd + manual tar parsing (no libarchive overhead)
#include "limcode/snapshot.h"
#include <zstd.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace limcode::snapshot;

// Tar header (512 bytes)
struct TarHeader {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
};

static_assert(sizeof(TarHeader) == 512, "TarHeader must be 512 bytes");

inline uint64_t parse_octal(const char* str, size_t len) {
    uint64_t result = 0;
    for (size_t i = 0; i < len && str[i] >= '0' && str[i] <= '7'; i++) {
        result = result * 8 + (str[i] - '0');
    }
    return result;
}

int main(int argc, char** argv) {
    std::string snapshot_path = argc > 1 ? argv[1] :
        "/home/larp/snapshots/snapshot-389758228.tar.zst";

    std::cout << "C++ ULTRA-FAST Snapshot Parser\n";
    std::cout << "Using: libzstd direct (no libarchive overhead)\n";
    std::cout << "Snapshot: " << snapshot_path << "\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    // Open file for streaming (no full load!)
    FILE* file = fopen(snapshot_path.c_str(), "rb");
    if (!file) {
        std::cerr << "Cannot open file\n";
        return 1;
    }

    std::cout << "Using streaming decompression (no full file load)...\n";

    // Create decompression stream
    ZSTD_DStream* dstream = ZSTD_createDStream();
    ZSTD_initDStream(dstream);

    // Input buffer (4MB chunks from disk)
    constexpr size_t IN_BUFFER_SIZE = 4 * 1024 * 1024;
    std::vector<uint8_t> in_buffer(IN_BUFFER_SIZE);

    // Output buffer (64MB decompressed chunks)
    constexpr size_t OUT_BUFFER_SIZE = 64 * 1024 * 1024;
    std::vector<uint8_t> out_buffer(OUT_BUFFER_SIZE);

    SnapshotStats stats;
    constexpr size_t HEADER_SIZE = sizeof(AppendVecHeader);

    std::vector<uint8_t> tar_buffer;
    tar_buffer.reserve(256 * 1024 * 1024); // 256MB reserve for tar data

    std::cout << "Decompressing and parsing...\n";

    // Stream decompress from file
    size_t bytes_read;
    while ((bytes_read = fread(in_buffer.data(), 1, IN_BUFFER_SIZE, file)) > 0) {
        ZSTD_inBuffer input = { in_buffer.data(), bytes_read, 0 };

        while (input.pos < input.size) {
            ZSTD_outBuffer output = { out_buffer.data(), OUT_BUFFER_SIZE, 0 };

            size_t ret = ZSTD_decompressStream(dstream, &output, &input);
            if (ZSTD_isError(ret)) {
                std::cerr << "Decompression error: " << ZSTD_getErrorName(ret) << "\n";
                goto done;
            }

            if (output.pos == 0) continue;

            // Append to tar buffer
            tar_buffer.insert(tar_buffer.end(), out_buffer.begin(), out_buffer.begin() + output.pos);
        }

        // Parse tar entries as we decompress
        size_t offset = 0;
        while (offset + 512 <= tar_buffer.size()) {
            const TarHeader* tar_hdr = reinterpret_cast<const TarHeader*>(tar_buffer.data() + offset);

            // Check for end of archive (empty header)
            if (tar_hdr->name[0] == 0) {
                break;
            }

            uint64_t file_size = parse_octal(tar_hdr->size, 12);
            size_t total_size = 512 + ((file_size + 511) / 512) * 512; // Header + aligned data

            if (offset + total_size > tar_buffer.size()) {
                break; // Need more data
            }

            // Check if this is an accounts/ file
            if (std::strncmp(tar_hdr->name, "accounts/", 9) == 0 && file_size > 0) {
                const uint8_t* data = tar_buffer.data() + offset + 512;

                // Parse AppendVec file (FAST PATH - inline everything)
                size_t acc_offset = 0;
                while (acc_offset + HEADER_SIZE <= file_size) {
                    const auto* hdr = reinterpret_cast<const AppendVecHeader*>(data + acc_offset);

                    if (acc_offset + HEADER_SIZE + hdr->data_len > file_size) {
                        break;
                    }

                    // Update stats (no function calls, pure inline)
                    stats.total_accounts++;
                    stats.total_lamports += hdr->lamports;
                    stats.total_data_bytes += hdr->data_len;

                    if (hdr->executable) {
                        stats.executable_accounts++;
                    }

                    if (hdr->data_len > stats.max_data_size) {
                        stats.max_data_size = hdr->data_len;
                    }

                    acc_offset += HEADER_SIZE + hdr->data_len;
                    acc_offset += (8 - (acc_offset % 8)) % 8; // Alignment
                }
            }

            offset += total_size;
        }

        // Remove processed tar data (keep remainder for next iteration)
        if (offset > 0) {
            tar_buffer.erase(tar_buffer.begin(), tar_buffer.begin() + offset);
        }

        // Progress
        if (stats.total_accounts % 10000000 == 0 && stats.total_accounts > 0) {
            std::cout << "Processed " << stats.total_accounts / 1000000 << "M accounts...\r" << std::flush;
        }
    }

done:
    ZSTD_freeDStream(dstream);
    fclose(file);

    auto end = std::chrono::high_resolution_clock::now();
    stats.parse_time_seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "\n\n=== RESULTS ===\n";
    std::cout << "Total accounts: " << stats.total_accounts << "\n";
    std::cout << "Total lamports: " << std::fixed << std::setprecision(2)
              << stats.total_sol() << " SOL\n";
    std::cout << "Total data: " << std::fixed << std::setprecision(2)
              << stats.total_data_mb() << " MB\n";
    std::cout << "Executable accounts: " << stats.executable_accounts << "\n";
    std::cout << "Data accounts: " << stats.data_accounts() << "\n";
    std::cout << "Max account size: " << stats.max_data_size << " bytes\n";
    std::cout << "Parse time: " << std::fixed << std::setprecision(2)
              << stats.parse_time_seconds << " seconds\n";
    std::cout << "Speed: " << std::fixed << std::setprecision(0)
              << stats.accounts_per_second() << " accounts/sec\n";

    // Compare to Rust
    double rust_time = 223.0;
    double speedup = rust_time / stats.parse_time_seconds;
    std::cout << "\nComparison to Rust: " << std::fixed << std::setprecision(2)
              << speedup << "x " << (speedup >= 1.0 ? "FASTER" : "slower") << "\n";

    return 0;
}

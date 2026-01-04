// FAST streaming: Zero-alloc with skip-ahead for non-accounts files
#include "limcode/snapshot.h"
#include <zstd.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>

using namespace limcode::snapshot;

struct TarHeader {
    char name[100]; char mode[8]; char uid[8]; char gid[8];
    char size[12]; char mtime[12]; char checksum[8]; char typeflag;
    char linkname[100]; char magic[6]; char version[2];
    char uname[32]; char gname[32]; char devmajor[8]; char devminor[8];
    char prefix[155]; char padding[12];
};

inline uint64_t parse_octal(const char* s, size_t n) {
    uint64_t r = 0;
    for (size_t i = 0; i < n && s[i] >= '0' && s[i] <= '7'; i++)
        r = r * 8 + (s[i] - '0');
    return r;
}

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "/home/larp/snapshots/snapshot-389758228.tar.zst";

    std::cout << "C++ FAST Snapshot Parser (zero-alloc streaming)\n";
    std::cout << "Snapshot: " << path << "\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { std::cerr << "Cannot open\n"; return 1; }

    ZSTD_DStream* ds = ZSTD_createDStream();
    ZSTD_initDStream(ds);

    constexpr size_t IN_SZ = 8 * 1024 * 1024;   // 8MB input chunks
    constexpr size_t OUT_SZ = 64 * 1024 * 1024; // 64MB output chunks
    constexpr size_t TAR_SZ = 256 * 1024 * 1024; // 256MB tar buffer (accounts files are < 128MB)

    uint8_t* in_buf = new uint8_t[IN_SZ];
    uint8_t* out_buf = new uint8_t[OUT_SZ];
    uint8_t* tar_buf = new uint8_t[TAR_SZ];

    size_t tar_len = 0;  // Valid data in tar_buf
    size_t tar_pos = 0;  // Current parse position
    size_t skip_bytes = 0; // Bytes to skip (for non-accounts files)

    SnapshotStats stats;
    constexpr size_t HDR_SZ = sizeof(AppendVecHeader);

    std::cout << "Parsing...\n";

    size_t bytes_read;
    while ((bytes_read = fread(in_buf, 1, IN_SZ, f)) > 0) {
        ZSTD_inBuffer in = { in_buf, bytes_read, 0 };

        while (in.pos < in.size) {
            ZSTD_outBuffer out = { out_buf, OUT_SZ, 0 };
            size_t ret = ZSTD_decompressStream(ds, &out, &in);
            if (ZSTD_isError(ret)) {
                std::cerr << "Decomp error: " << ZSTD_getErrorName(ret) << "\n";
                goto done;
            }

            if (out.pos == 0) continue;

            // Handle skipping non-accounts data
            size_t data_start = 0;
            if (skip_bytes > 0) {
                if (out.pos <= skip_bytes) {
                    skip_bytes -= out.pos;
                    continue; // Skip entire chunk
                }
                data_start = skip_bytes;
                skip_bytes = 0;
                // Reset buffer - we've finished skipping
                tar_len = 0;
                tar_pos = 0;
            }

            size_t new_data = out.pos - data_start;

            // Check if we need to compact
            if (tar_len + new_data > TAR_SZ) {
                size_t unprocessed = tar_len - tar_pos;
                if (unprocessed > 0) {
                    memmove(tar_buf, tar_buf + tar_pos, unprocessed);
                }
                tar_len = unprocessed;
                tar_pos = 0;

                if (tar_len + new_data > TAR_SZ) {
                    std::cerr << "Fatal: accounts file too large\n";
                    goto done;
                }
            }

            // Append decompressed data
            memcpy(tar_buf + tar_len, out_buf + data_start, new_data);
            tar_len += new_data;

            // Parse tar entries
            while (tar_pos + 512 <= tar_len) {
                const TarHeader* th = reinterpret_cast<const TarHeader*>(tar_buf + tar_pos);
                if (th->name[0] == 0) goto done; // End of archive

                uint64_t fsz = parse_octal(th->size, 12);
                size_t tot = 512 + ((fsz + 511) / 512) * 512;

                // Check if this is an accounts file
                bool is_accounts = strncmp(th->name, "accounts/", 9) == 0 && fsz > 0;

                if (!is_accounts) {
                    // Skip non-accounts files
                    if (tar_pos + tot <= tar_len) {
                        // We have the whole thing, just advance
                        tar_pos += tot;
                    } else {
                        // Need to skip more data from future chunks
                        size_t have = tar_len - tar_pos;
                        skip_bytes = tot - have;
                        tar_pos = tar_len; // Consumed all current data
                    }
                    continue;
                }

                // It's an accounts file - need full data
                if (tar_pos + tot > tar_len) break; // Wait for more data

                // Parse AppendVec
                const uint8_t* d = tar_buf + tar_pos + 512;
                size_t off = 0;
                while (off + HDR_SZ <= fsz) {
                    const auto* h = reinterpret_cast<const AppendVecHeader*>(d + off);
                    if (off + HDR_SZ + h->data_len > fsz) break;

                    stats.total_accounts++;
                    stats.total_lamports += h->lamports;
                    stats.total_data_bytes += h->data_len;
                    if (h->executable) stats.executable_accounts++;
                    if (h->data_len > stats.max_data_size) stats.max_data_size = h->data_len;

                    off += HDR_SZ + h->data_len;
                    off += (8 - (off % 8)) % 8;
                }

                tar_pos += tot;
            }

            // Progress
            if (stats.total_accounts % 10000000 == 0 && stats.total_accounts > 0) {
                std::cout << stats.total_accounts / 1000000 << "M accounts...\r" << std::flush;
            }
        }
    }

done:
    ZSTD_freeDStream(ds);
    fclose(f);
    delete[] in_buf;
    delete[] out_buf;
    delete[] tar_buf;

    auto end = std::chrono::high_resolution_clock::now();
    stats.parse_time_seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "\n\n=== RESULTS ===\n";
    std::cout << "Accounts: " << stats.total_accounts << "\n";
    std::cout << "SOL: " << std::fixed << std::setprecision(2) << stats.total_sol() << "\n";
    std::cout << "Data: " << stats.total_data_mb() << " MB\n";
    std::cout << "Exec: " << stats.executable_accounts << "\n";
    std::cout << "Time: " << stats.parse_time_seconds << "s\n";
    std::cout << "Speed: " << std::fixed << std::setprecision(0) << stats.accounts_per_second() << " acc/s\n";
    std::cout << "\nvs Rust (223s): " << std::setprecision(2) << (223.0 / stats.parse_time_seconds) << "x\n";

    return 0;
}

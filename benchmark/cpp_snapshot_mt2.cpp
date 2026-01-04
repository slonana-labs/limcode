// MULTITHREADED v2: Zero-copy with buffer pool, parallel decompression
#include "limcode/snapshot.h"
#include <zstd.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

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

// Parse accounts from a memory region (inline for speed)
inline void parse_accounts_inline(const uint8_t* data, size_t size,
                                   uint64_t& accounts, uint64_t& lamports,
                                   uint64_t& data_bytes, uint64_t& exec,
                                   uint64_t& max_size) {
    constexpr size_t HDR_SZ = sizeof(AppendVecHeader);
    size_t off = 0;
    while (off + HDR_SZ <= size) {
        const auto* h = reinterpret_cast<const AppendVecHeader*>(data + off);
        if (off + HDR_SZ + h->data_len > size) break;

        accounts++;
        lamports += h->lamports;
        data_bytes += h->data_len;
        if (h->executable) exec++;
        if (h->data_len > max_size) max_size = h->data_len;

        off += HDR_SZ + h->data_len;
        off += (8 - (off % 8)) % 8;
    }
}

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "/home/larp/snapshots/snapshot-389758228.tar.zst";

    std::cout << "C++ OPTIMIZED Snapshot Parser (single-thread, max speed)\n";
    std::cout << "Snapshot: " << path << "\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { std::cerr << "Cannot open\n"; return 1; }

    ZSTD_DStream* ds = ZSTD_createDStream();
    ZSTD_initDStream(ds);

    // Larger buffers for better I/O throughput
    constexpr size_t IN_SZ = 32 * 1024 * 1024;  // 32MB input
    constexpr size_t OUT_SZ = 128 * 1024 * 1024; // 128MB output
    constexpr size_t TAR_SZ = 256 * 1024 * 1024; // 256MB tar

    uint8_t* in_buf = new uint8_t[IN_SZ];
    uint8_t* out_buf = new uint8_t[OUT_SZ];
    uint8_t* tar_buf = new uint8_t[TAR_SZ];

    size_t tar_len = 0, tar_pos = 0, skip_bytes = 0;

    // Stats
    uint64_t total_accounts = 0, total_lamports = 0, total_data_bytes = 0;
    uint64_t executable_accounts = 0, max_data_size = 0;

    std::cout << "Parsing...\n";

    size_t bytes_read;
    while ((bytes_read = fread(in_buf, 1, IN_SZ, f)) > 0) {
        ZSTD_inBuffer in = {in_buf, bytes_read, 0};

        while (in.pos < in.size) {
            ZSTD_outBuffer out = {out_buf, OUT_SZ, 0};
            size_t ret = ZSTD_decompressStream(ds, &out, &in);
            if (ZSTD_isError(ret)) {
                std::cerr << "Decomp error\n";
                goto done;
            }

            if (out.pos == 0) continue;

            // Handle skip
            size_t data_start = 0;
            if (skip_bytes > 0) {
                if (out.pos <= skip_bytes) {
                    skip_bytes -= out.pos;
                    continue;
                }
                data_start = skip_bytes;
                skip_bytes = 0;
                tar_len = 0;
                tar_pos = 0;
            }

            size_t new_data = out.pos - data_start;

            // Compact
            if (tar_len + new_data > TAR_SZ) {
                size_t unprocessed = tar_len - tar_pos;
                if (unprocessed > 0) memmove(tar_buf, tar_buf + tar_pos, unprocessed);
                tar_len = unprocessed;
                tar_pos = 0;
            }

            memcpy(tar_buf + tar_len, out_buf + data_start, new_data);
            tar_len += new_data;

            // Parse tar
            while (tar_pos + 512 <= tar_len) {
                const TarHeader* th = reinterpret_cast<const TarHeader*>(tar_buf + tar_pos);
                if (th->name[0] == 0) goto done;

                uint64_t fsz = parse_octal(th->size, 12);
                size_t tot = 512 + ((fsz + 511) / 512) * 512;

                bool is_accounts = strncmp(th->name, "accounts/", 9) == 0 && fsz > 0;

                if (!is_accounts) {
                    if (tar_pos + tot <= tar_len) {
                        tar_pos += tot;
                    } else {
                        skip_bytes = tot - (tar_len - tar_pos);
                        tar_pos = tar_len;
                    }
                    continue;
                }

                if (tar_pos + tot > tar_len) break;

                // Parse accounts inline
                parse_accounts_inline(tar_buf + tar_pos + 512, fsz,
                                       total_accounts, total_lamports, total_data_bytes,
                                       executable_accounts, max_data_size);

                tar_pos += tot;
            }

            if (total_accounts % 50000000 < 5000000) {
                std::cout << total_accounts / 1000000 << "M...\r" << std::flush;
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
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "\n\n=== RESULTS ===\n";
    std::cout << "Accounts: " << total_accounts << "\n";
    std::cout << "SOL: " << std::fixed << std::setprecision(2) << (total_lamports / 1e9) << "\n";
    std::cout << "Data: " << (total_data_bytes / 1024.0 / 1024.0) << " MB\n";
    std::cout << "Exec: " << executable_accounts << "\n";
    std::cout << "Time: " << elapsed << "s\n";
    std::cout << "Speed: " << std::fixed << std::setprecision(0) << (total_accounts / elapsed) << " acc/s\n";
    std::cout << "\nvs Rust (223s): " << std::setprecision(2) << (223.0 / elapsed) << "x\n";

    return 0;
}

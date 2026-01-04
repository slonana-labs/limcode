// MMAP INPUT: mmap compressed file for zero-copy I/O
#include "limcode/snapshot.h"
#include <zstd.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

    std::cout << "C++ MMAP-INPUT Snapshot Parser (mmap compressed file)\n";
    std::cout << "Snapshot: " << path << "\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    // mmap the compressed file
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) { std::cerr << "Cannot open\n"; return 1; }

    struct stat st;
    fstat(fd, &st);
    size_t file_size = st.st_size;
    std::cout << "Compressed size: " << file_size / 1024 / 1024 << " MB\n";

    const uint8_t* compressed = static_cast<const uint8_t*>(
        mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0)
    );
    if (compressed == MAP_FAILED) {
        std::cerr << "mmap failed\n";
        close(fd);
        return 1;
    }
    madvise((void*)compressed, file_size, MADV_SEQUENTIAL);

    ZSTD_DStream* ds = ZSTD_createDStream();
    ZSTD_initDStream(ds);

    constexpr size_t OUT_SZ = 64 * 1024 * 1024; // 64MB output
    constexpr size_t TAR_SZ = 256 * 1024 * 1024; // 256MB tar

    uint8_t* out_buf = new uint8_t[OUT_SZ];
    uint8_t* tar_buf = new uint8_t[TAR_SZ];

    size_t tar_len = 0, tar_pos = 0, skip_bytes = 0;
    size_t in_pos = 0;
    constexpr size_t HDR_SZ = sizeof(AppendVecHeader);

    uint64_t total_accounts = 0, total_lamports = 0, total_data_bytes = 0;
    uint64_t executable_accounts = 0, max_data_size = 0;

    std::cout << "Parsing...\n";

    while (in_pos < file_size) {
        // Use large input chunks from mmap'd memory
        size_t chunk = std::min(file_size - in_pos, (size_t)(16 * 1024 * 1024));
        ZSTD_inBuffer in = {compressed + in_pos, chunk, 0};

        while (in.pos < in.size) {
            ZSTD_outBuffer out = {out_buf, OUT_SZ, 0};
            size_t ret = ZSTD_decompressStream(ds, &out, &in);
            if (ZSTD_isError(ret)) {
                std::cerr << "Decomp error: " << ZSTD_getErrorName(ret) << "\n";
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

                // Parse accounts
                const uint8_t* d = tar_buf + tar_pos + 512;
                size_t off = 0;
                while (off + HDR_SZ <= fsz) {
                    const auto* h = reinterpret_cast<const AppendVecHeader*>(d + off);
                    if (off + HDR_SZ + h->data_len > fsz) break;

                    total_accounts++;
                    total_lamports += h->lamports;
                    total_data_bytes += h->data_len;
                    if (h->executable) executable_accounts++;
                    if (h->data_len > max_data_size) max_data_size = h->data_len;

                    off += HDR_SZ + h->data_len;
                    off += (8 - (off % 8)) % 8;
                }

                tar_pos += tot;
            }

            if (total_accounts % 50000000 < 1000000) {
                std::cout << total_accounts / 1000000 << "M...\r" << std::flush;
            }
        }

        in_pos += in.pos;
    }

done:
    ZSTD_freeDStream(ds);
    munmap((void*)compressed, file_size);
    close(fd);
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
    std::cout << "vs C++ ST (198s): " << (198.0 / elapsed) << "x\n";

    return 0;
}

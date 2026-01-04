// MMAP-based approach: decompress to temp file, then mmap
// This avoids all the vector overhead
#include "limcode/snapshot.h"
#include <zstd.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <atomic>

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

    std::cout << "C++ MMAP Snapshot Parser (decompress-then-parse)\n";
    std::cout << "Snapshot: " << snapshot_path << "\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    // Step 1: Decompress to temp file
    std::string tmp_path = "/tmp/limcode_snapshot_" + std::to_string(getpid()) + ".tar";

    std::cout << "Step 1: Decompressing to " << tmp_path << "...\n";
    auto decomp_start = std::chrono::high_resolution_clock::now();

    // Use zstd CLI for fastest decompression (uses all cores)
    std::string cmd = "zstd -d -o " + tmp_path + " " + snapshot_path + " --force 2>&1";
    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Decompression failed\n";
        return 1;
    }

    auto decomp_end = std::chrono::high_resolution_clock::now();
    double decomp_time = std::chrono::duration<double>(decomp_end - decomp_start).count();
    std::cout << "Decompression time: " << std::fixed << std::setprecision(2) << decomp_time << "s\n";

    // Step 2: Memory-map the decompressed tar file
    std::cout << "Step 2: Memory-mapping tar file...\n";

    int fd = open(tmp_path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Cannot open temp file\n";
        return 1;
    }

    struct stat st;
    fstat(fd, &st);
    size_t file_size = st.st_size;
    std::cout << "Decompressed size: " << file_size / 1024 / 1024 << " MB\n";

    const uint8_t* data = static_cast<const uint8_t*>(
        mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0)
    );

    if (data == MAP_FAILED) {
        std::cerr << "mmap failed\n";
        close(fd);
        return 1;
    }

    // Advise kernel for sequential access
    madvise((void*)data, file_size, MADV_SEQUENTIAL | MADV_WILLNEED);

    // Step 3: Parse tar + accounts (FAST - everything is in memory)
    std::cout << "Step 3: Parsing accounts...\n";
    auto parse_start = std::chrono::high_resolution_clock::now();

    SnapshotStats stats;
    constexpr size_t HEADER_SIZE = sizeof(AppendVecHeader);

    size_t offset = 0;
    while (offset + 512 <= file_size) {
        const TarHeader* tar_hdr = reinterpret_cast<const TarHeader*>(data + offset);

        // End of archive
        if (tar_hdr->name[0] == 0) break;

        uint64_t entry_size = parse_octal(tar_hdr->size, 12);
        size_t total_size = 512 + ((entry_size + 511) / 512) * 512;

        if (offset + total_size > file_size) break;

        // Check for accounts/ files
        if (std::strncmp(tar_hdr->name, "accounts/", 9) == 0 && entry_size > 0) {
            const uint8_t* acc_data = data + offset + 512;

            // Parse AppendVec (ULTRA FAST - direct memory access)
            size_t acc_offset = 0;
            while (acc_offset + HEADER_SIZE <= entry_size) {
                const auto* hdr = reinterpret_cast<const AppendVecHeader*>(acc_data + acc_offset);

                if (acc_offset + HEADER_SIZE + hdr->data_len > entry_size) break;

                stats.total_accounts++;
                stats.total_lamports += hdr->lamports;
                stats.total_data_bytes += hdr->data_len;

                if (hdr->executable) stats.executable_accounts++;
                if (hdr->data_len > stats.max_data_size) stats.max_data_size = hdr->data_len;

                acc_offset += HEADER_SIZE + hdr->data_len;
                acc_offset += (8 - (acc_offset % 8)) % 8;
            }
        }

        offset += total_size;

        // Progress
        if (stats.total_accounts % 50000000 == 0 && stats.total_accounts > 0) {
            std::cout << "Parsed " << stats.total_accounts / 1000000 << "M accounts...\r" << std::flush;
        }
    }

    auto parse_end = std::chrono::high_resolution_clock::now();
    double parse_time = std::chrono::duration<double>(parse_end - parse_start).count();

    // Cleanup
    munmap((void*)data, file_size);
    close(fd);
    unlink(tmp_path.c_str());

    auto end = std::chrono::high_resolution_clock::now();
    stats.parse_time_seconds = std::chrono::duration<double>(end - start).count();

    std::cout << "\n\n=== RESULTS ===\n";
    std::cout << "Total accounts: " << stats.total_accounts << "\n";
    std::cout << "Total lamports: " << std::fixed << std::setprecision(2) << stats.total_sol() << " SOL\n";
    std::cout << "Total data: " << std::fixed << std::setprecision(2) << stats.total_data_mb() << " MB\n";
    std::cout << "Executable: " << stats.executable_accounts << "\n";
    std::cout << "Data accounts: " << stats.data_accounts() << "\n";
    std::cout << "Max size: " << stats.max_data_size << " bytes\n\n";

    std::cout << "Decompression: " << std::fixed << std::setprecision(2) << decomp_time << "s\n";
    std::cout << "Parsing only: " << std::fixed << std::setprecision(2) << parse_time << "s\n";
    std::cout << "Total time: " << std::fixed << std::setprecision(2) << stats.parse_time_seconds << "s\n";
    std::cout << "Parse speed: " << std::fixed << std::setprecision(0) << (stats.total_accounts / parse_time) << " accounts/sec\n";
    std::cout << "Overall speed: " << std::fixed << std::setprecision(0) << stats.accounts_per_second() << " accounts/sec\n";

    double rust_time = 223.0;
    double speedup = rust_time / stats.parse_time_seconds;
    std::cout << "\nvs Rust: " << std::fixed << std::setprecision(2) << speedup << "x "
              << (speedup >= 1.0 ? "FASTER!" : "slower") << "\n";

    return 0;
}

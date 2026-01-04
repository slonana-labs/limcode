// PIPE: Use zstd CLI (multithreaded) + parallel parsing
#include "limcode/snapshot.h"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>

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

// Work chunk for parallel parsing
struct Chunk {
    const uint8_t* data;
    size_t size;
};

std::atomic<uint64_t> g_accounts{0}, g_lamports{0}, g_data_bytes{0}, g_exec{0}, g_max{0};

void parse_chunk(const uint8_t* data, size_t size) {
    constexpr size_t HDR_SZ = sizeof(AppendVecHeader);
    uint64_t acc = 0, lam = 0, db = 0, ex = 0, mx = 0;

    size_t off = 0;
    while (off + HDR_SZ <= size) {
        const auto* h = reinterpret_cast<const AppendVecHeader*>(data + off);
        if (off + HDR_SZ + h->data_len > size) break;

        acc++;
        lam += h->lamports;
        db += h->data_len;
        if (h->executable) ex++;
        if (h->data_len > mx) mx = h->data_len;

        off += HDR_SZ + h->data_len;
        off += (8 - (off % 8)) % 8;
    }

    g_accounts += acc;
    g_lamports += lam;
    g_data_bytes += db;
    g_exec += ex;

    uint64_t cur = g_max;
    while (mx > cur && !g_max.compare_exchange_weak(cur, mx));
}

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "/home/larp/snapshots/snapshot-389758228.tar.zst";
    unsigned num_threads = std::thread::hardware_concurrency();

    std::cout << "C++ PIPE Parser (zstd CLI + parallel parse)\n";
    std::cout << "Threads: " << num_threads << "\n";
    std::cout << "Snapshot: " << path << "\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    // Use zstd CLI for multithreaded decompression via pipe
    std::string cmd = "zstd -d -c --stdout " + path;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) { std::cerr << "Cannot open pipe\n"; return 1; }

    constexpr size_t BUF_SZ = 256 * 1024 * 1024; // 256MB buffer
    uint8_t* tar_buf = new uint8_t[BUF_SZ];

    size_t tar_len = 0, tar_pos = 0;

    std::cout << "Parsing...\n";

    // Read decompressed data in large chunks
    constexpr size_t READ_SZ = 64 * 1024 * 1024; // 64MB reads
    uint8_t* read_buf = new uint8_t[READ_SZ];

    std::vector<std::thread> workers;
    std::mutex work_mutex;
    std::vector<Chunk> work_queue;
    std::atomic<bool> done{false};
    std::atomic<size_t> pending{0};

    // Worker threads
    auto worker = [&]() {
        while (true) {
            Chunk chunk{nullptr, 0};
            {
                std::lock_guard<std::mutex> lock(work_mutex);
                if (!work_queue.empty()) {
                    chunk = work_queue.back();
                    work_queue.pop_back();
                }
            }
            if (chunk.data) {
                parse_chunk(chunk.data, chunk.size);
                pending--;
            } else if (done) {
                break;
            } else {
                std::this_thread::yield();
            }
        }
    };

    for (unsigned i = 0; i < num_threads; i++) {
        workers.emplace_back(worker);
    }

    size_t bytes_read;
    while ((bytes_read = fread(read_buf, 1, READ_SZ, pipe)) > 0) {
        // Compact if needed
        if (tar_len + bytes_read > BUF_SZ) {
            // Wait for pending work that uses current buffer
            while (pending > 0) std::this_thread::yield();

            size_t unprocessed = tar_len - tar_pos;
            if (unprocessed > 0) memmove(tar_buf, tar_buf + tar_pos, unprocessed);
            tar_len = unprocessed;
            tar_pos = 0;
        }

        memcpy(tar_buf + tar_len, read_buf, bytes_read);
        tar_len += bytes_read;

        // Parse tar entries
        while (tar_pos + 512 <= tar_len) {
            const TarHeader* th = reinterpret_cast<const TarHeader*>(tar_buf + tar_pos);
            if (th->name[0] == 0) goto finish;

            uint64_t fsz = parse_octal(th->size, 12);
            size_t tot = 512 + ((fsz + 511) / 512) * 512;

            if (tar_pos + tot > tar_len) break;

            bool is_accounts = strncmp(th->name, "accounts/", 9) == 0 && fsz > 0;

            if (is_accounts) {
                // Queue for parallel parsing
                {
                    std::lock_guard<std::mutex> lock(work_mutex);
                    work_queue.push_back({tar_buf + tar_pos + 512, fsz});
                    pending++;
                }
            }

            tar_pos += tot;
        }

        uint64_t acc = g_accounts;
        if (acc % 50000000 < 5000000) {
            std::cout << acc / 1000000 << "M...\r" << std::flush;
        }
    }

finish:
    // Wait for remaining work
    while (pending > 0) std::this_thread::yield();
    done = true;

    for (auto& t : workers) t.join();

    pclose(pipe);
    delete[] tar_buf;
    delete[] read_buf;

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "\n\n=== RESULTS ===\n";
    std::cout << "Accounts: " << g_accounts << "\n";
    std::cout << "SOL: " << std::fixed << std::setprecision(2) << (g_lamports / 1e9) << "\n";
    std::cout << "Data: " << (g_data_bytes / 1024.0 / 1024.0) << " MB\n";
    std::cout << "Exec: " << g_exec << "\n";
    std::cout << "Time: " << elapsed << "s\n";
    std::cout << "Speed: " << std::fixed << std::setprecision(0) << (g_accounts / elapsed) << " acc/s\n";
    std::cout << "\nvs Rust (223s): " << std::setprecision(2) << (223.0 / elapsed) << "x\n";

    return 0;
}

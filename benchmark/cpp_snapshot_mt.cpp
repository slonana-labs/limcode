// MULTITHREADED: Parallel account parsing with worker threads
#include "limcode/snapshot.h"
#include <zstd.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
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

// Work item for parallel processing
struct WorkItem {
    std::vector<uint8_t> data;
    size_t size;
};

// Thread-safe work queue
class WorkQueue {
    std::queue<WorkItem> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> done_{false};
    std::atomic<size_t> pending_{0};

public:
    void push(WorkItem&& item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
            pending_++;
        }
        cv_.notify_one();
    }

    bool pop(WorkItem& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || done_; });
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void finish() {
        done_ = true;
        cv_.notify_all();
    }

    void complete_one() { pending_--; }
    size_t pending() const { return pending_; }
};

// Thread-local stats that get merged
struct ThreadStats {
    uint64_t total_accounts = 0;
    uint64_t total_lamports = 0;
    uint64_t total_data_bytes = 0;
    uint64_t executable_accounts = 0;
    uint64_t max_data_size = 0;
};

// Global stats with atomic updates
std::atomic<uint64_t> g_total_accounts{0};
std::atomic<uint64_t> g_total_lamports{0};
std::atomic<uint64_t> g_total_data_bytes{0};
std::atomic<uint64_t> g_executable_accounts{0};
std::atomic<uint64_t> g_max_data_size{0};

void worker_thread(WorkQueue& queue) {
    constexpr size_t HDR_SZ = sizeof(AppendVecHeader);
    ThreadStats local;

    WorkItem item;
    while (queue.pop(item)) {
        const uint8_t* d = item.data.data();
        size_t fsz = item.size;
        size_t off = 0;

        while (off + HDR_SZ <= fsz) {
            const auto* h = reinterpret_cast<const AppendVecHeader*>(d + off);
            if (off + HDR_SZ + h->data_len > fsz) break;

            local.total_accounts++;
            local.total_lamports += h->lamports;
            local.total_data_bytes += h->data_len;
            if (h->executable) local.executable_accounts++;
            if (h->data_len > local.max_data_size) local.max_data_size = h->data_len;

            off += HDR_SZ + h->data_len;
            off += (8 - (off % 8)) % 8;
        }

        queue.complete_one();

        // Batch update globals every 1M accounts to reduce contention
        if (local.total_accounts >= 1000000) {
            g_total_accounts += local.total_accounts;
            g_total_lamports += local.total_lamports;
            g_total_data_bytes += local.total_data_bytes;
            g_executable_accounts += local.executable_accounts;

            uint64_t cur_max = g_max_data_size;
            while (local.max_data_size > cur_max &&
                   !g_max_data_size.compare_exchange_weak(cur_max, local.max_data_size));

            local = ThreadStats{};
        }
    }

    // Final flush
    g_total_accounts += local.total_accounts;
    g_total_lamports += local.total_lamports;
    g_total_data_bytes += local.total_data_bytes;
    g_executable_accounts += local.executable_accounts;

    uint64_t cur_max = g_max_data_size;
    while (local.max_data_size > cur_max &&
           !g_max_data_size.compare_exchange_weak(cur_max, local.max_data_size));
}

int main(int argc, char** argv) {
    std::string path = argc > 1 ? argv[1] : "/home/larp/snapshots/snapshot-389758228.tar.zst";
    unsigned num_threads = std::thread::hardware_concurrency();
    if (argc > 2) num_threads = std::stoi(argv[2]);

    std::cout << "C++ MULTITHREADED Snapshot Parser\n";
    std::cout << "Threads: " << num_threads << "\n";
    std::cout << "Snapshot: " << path << "\n\n";

    auto start = std::chrono::high_resolution_clock::now();

    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { std::cerr << "Cannot open\n"; return 1; }

    // Use multithreaded zstd decompression
    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    ZSTD_DCtx_setParameter(dctx, ZSTD_d_windowLogMax, 31);

    constexpr size_t IN_SZ = 16 * 1024 * 1024;  // 16MB input chunks
    constexpr size_t OUT_SZ = 64 * 1024 * 1024; // 64MB output chunks
    constexpr size_t TAR_SZ = 256 * 1024 * 1024; // 256MB tar buffer

    uint8_t* in_buf = new uint8_t[IN_SZ];
    uint8_t* out_buf = new uint8_t[OUT_SZ];
    uint8_t* tar_buf = new uint8_t[TAR_SZ];

    size_t tar_len = 0;
    size_t tar_pos = 0;
    size_t skip_bytes = 0;

    // Start worker threads
    WorkQueue queue;
    std::vector<std::thread> workers;
    for (unsigned i = 0; i < num_threads; i++) {
        workers.emplace_back(worker_thread, std::ref(queue));
    }

    std::cout << "Parsing...\n";
    size_t files_queued = 0;

    size_t bytes_read;
    ZSTD_inBuffer in = {nullptr, 0, 0};

    while (true) {
        // Read more compressed data if needed
        if (in.pos >= in.size) {
            bytes_read = fread(in_buf, 1, IN_SZ, f);
            if (bytes_read == 0) break;
            in = {in_buf, bytes_read, 0};
        }

        ZSTD_outBuffer out = {out_buf, OUT_SZ, 0};
        size_t ret = ZSTD_decompressStream(dctx, &out, &in);
        if (ZSTD_isError(ret)) {
            std::cerr << "Decomp error: " << ZSTD_getErrorName(ret) << "\n";
            break;
        }

        if (out.pos == 0) continue;

        // Handle skipping
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

        // Compact if needed
        if (tar_len + new_data > TAR_SZ) {
            size_t unprocessed = tar_len - tar_pos;
            if (unprocessed > 0) {
                memmove(tar_buf, tar_buf + tar_pos, unprocessed);
            }
            tar_len = unprocessed;
            tar_pos = 0;
        }

        memcpy(tar_buf + tar_len, out_buf + data_start, new_data);
        tar_len += new_data;

        // Parse tar entries
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

            // Queue work for parallel processing
            WorkItem item;
            item.data.assign(tar_buf + tar_pos + 512, tar_buf + tar_pos + 512 + fsz);
            item.size = fsz;
            queue.push(std::move(item));
            files_queued++;

            tar_pos += tot;
        }

        // Progress
        uint64_t acc = g_total_accounts;
        if (acc % 50000000 < 1000000) {
            std::cout << acc / 1000000 << "M accounts, "
                      << files_queued << " files queued, "
                      << queue.pending() << " pending...\r" << std::flush;
        }
    }

done:
    // Wait for all work to complete
    queue.finish();
    for (auto& t : workers) t.join();

    ZSTD_freeDCtx(dctx);
    fclose(f);
    delete[] in_buf;
    delete[] out_buf;
    delete[] tar_buf;

    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "\n\n=== RESULTS ===\n";
    std::cout << "Accounts: " << g_total_accounts << "\n";
    std::cout << "SOL: " << std::fixed << std::setprecision(2)
              << (g_total_lamports / 1e9) << "\n";
    std::cout << "Data: " << std::setprecision(2)
              << (g_total_data_bytes / 1024.0 / 1024.0) << " MB\n";
    std::cout << "Exec: " << g_executable_accounts << "\n";
    std::cout << "Time: " << elapsed << "s\n";
    std::cout << "Speed: " << std::fixed << std::setprecision(0)
              << (g_total_accounts / elapsed) << " acc/s\n";
    std::cout << "Files: " << files_queued << "\n";
    std::cout << "\nvs Rust (223s): " << std::setprecision(2)
              << (223.0 / elapsed) << "x\n";
    std::cout << "vs C++ ST (198s): " << std::setprecision(2)
              << (198.0 / elapsed) << "x\n";

    return 0;
}

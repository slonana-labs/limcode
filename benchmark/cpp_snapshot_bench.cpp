#include "limcode/snapshot.h"
#include <iostream>
#include <iomanip>

using namespace limcode::snapshot;

int main(int argc, char** argv) {
    std::string snapshot_path = argc > 1
        ? argv[1]
        : "/home/larp/snapshots/snapshot-389758228.tar.zst";

    std::cout << "C++ Snapshot Parser - Speed Benchmark\n";
    std::cout << "Snapshot: " << snapshot_path << "\n\n";

    SnapshotStats stats;

    std::cout << "Parsing (optimized - no callbacks, no printing)...\n";

    int64_t count = parse_snapshot_stats(snapshot_path, stats);

    if (count < 0) {
        std::cerr << "Error parsing snapshot\n";
        return 1;
    }

    std::cout << "\n=== RESULTS ===\n";
    std::cout << "Total accounts: " << stats.total_accounts << "\n";
    std::cout << "Total lamports: " << std::fixed << std::setprecision(2)
              << stats.total_sol() << " SOL\n";
    std::cout << "Total data: " << std::fixed << std::setprecision(2)
              << stats.total_data_mb() << " MB\n";
    std::cout << "Executable accounts: " << stats.executable_accounts << "\n";
    std::cout << "Data accounts: " << stats.data_accounts() << "\n";
    std::cout << "Max account data size: " << stats.max_data_size << " bytes\n";
    std::cout << "Parse time: " << std::fixed << std::setprecision(2)
              << stats.parse_time_seconds << " seconds\n";
    std::cout << "Speed: " << std::fixed << std::setprecision(0)
              << stats.accounts_per_second() << " accounts/sec\n";

    return 0;
}

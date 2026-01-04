#include "limcode/snapshot.h"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace limcode::snapshot;

std::string hex(const uint8_t* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; i++) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

int main(int argc, char** argv) {
    std::string snapshot_path = argc > 1
        ? argv[1]
        : "/home/larp/larpdevs/osvm-cli/test-ledger/snapshot-1388137-HESbqSY6jVvngkUVSiBMCXY5iYaa1bfo1ApRgCRihJVq.tar.zst";

    std::cout << "C++ Solana Snapshot Parser Test\n";
    std::cout << "Snapshot: " << snapshot_path << "\n\n";

    // Test streaming API (recommended for large snapshots)
    SnapshotStats stats;
    size_t shown = 0;

    int64_t count = stream_snapshot(snapshot_path, [&](const SnapshotAccount& acc) {
        // Show first 10 accounts
        if (shown < 10) {
            std::cout << "Account " << shown << ":\n";
            std::cout << "  Pubkey: " << hex(acc.pubkey.data(), 32) << "\n";
            std::cout << "  Lamports: " << acc.lamports << "\n";
            std::cout << "  Owner: " << hex(acc.owner.data(), 32) << "\n";
            std::cout << "  Executable: " << (acc.executable ? "true" : "false") << "\n";
            std::cout << "  Rent epoch: " << acc.rent_epoch << "\n";
            std::cout << "  Hash: " << hex(acc.hash.data(), 32) << "\n";
            std::cout << "  Data size: " << acc.data.size() << " bytes\n";
            std::cout << "  Write version: " << acc.write_version << "\n\n";
            shown++;
        }

        // Collect stats
        stats.total_accounts++;
        stats.total_lamports += acc.lamports;
        stats.total_data_bytes += acc.data.size();

        if (acc.executable) {
            stats.executable_accounts++;
        }

        if (acc.data.size() > stats.max_data_size) {
            stats.max_data_size = acc.data.size();
        }

        // Progress every 10k accounts
        if (stats.total_accounts % 10000 == 0) {
            std::cout << "Processed " << stats.total_accounts << " accounts...\r" << std::flush;
        }

        return true; // Continue
    });

    if (count < 0) {
        std::cerr << "Error parsing snapshot\n";
        return 1;
    }

    std::cout << "\n\n=== RESULTS ===\n";
    std::cout << "Total accounts: " << stats.total_accounts << "\n";
    std::cout << "Total lamports: " << std::fixed << std::setprecision(2)
              << stats.total_sol() << " SOL\n";
    std::cout << "Total data: " << std::fixed << std::setprecision(2)
              << stats.total_data_mb() << " MB\n";
    std::cout << "Executable accounts: " << stats.executable_accounts << "\n";
    std::cout << "Data accounts: " << stats.data_accounts() << "\n";
    std::cout << "Max account data size: " << stats.max_data_size << " bytes\n";

    return 0;
}

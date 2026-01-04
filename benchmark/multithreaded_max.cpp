#include <immintrin.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>
#include <vector>
#include <atomic>

using namespace std::chrono;

// Per-thread benchmark
double benchmark_thread(size_t num_elements, size_t iterations) {
    const size_t data_bytes = num_elements * sizeof(uint64_t);

    // Aligned allocation
    uint64_t* data = (uint64_t*)aligned_alloc(64, data_bytes);
    uint8_t* buffer = (uint8_t*)aligned_alloc(64, data_bytes + 64);

    for (size_t i = 0; i < num_elements; ++i) {
        data[i] = 0xABCDEF0123456789ULL + i;
    }

    // Warmup
    for (size_t i = 0; i < 10; ++i) {
        *reinterpret_cast<uint64_t*>(buffer) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buffer + 8);

        size_t chunks = data_bytes / 64;
        size_t j = 0;
        for (; j + 16 <= chunks; j += 16) {
            __m512i v0 = _mm512_loadu_si512(s+j);
            __m512i v1 = _mm512_loadu_si512(s+j+1);
            __m512i v2 = _mm512_loadu_si512(s+j+2);
            __m512i v3 = _mm512_loadu_si512(s+j+3);
            __m512i v4 = _mm512_loadu_si512(s+j+4);
            __m512i v5 = _mm512_loadu_si512(s+j+5);
            __m512i v6 = _mm512_loadu_si512(s+j+6);
            __m512i v7 = _mm512_loadu_si512(s+j+7);
            __m512i v8 = _mm512_loadu_si512(s+j+8);
            __m512i v9 = _mm512_loadu_si512(s+j+9);
            __m512i v10 = _mm512_loadu_si512(s+j+10);
            __m512i v11 = _mm512_loadu_si512(s+j+11);
            __m512i v12 = _mm512_loadu_si512(s+j+12);
            __m512i v13 = _mm512_loadu_si512(s+j+13);
            __m512i v14 = _mm512_loadu_si512(s+j+14);
            __m512i v15 = _mm512_loadu_si512(s+j+15);

            _mm512_storeu_si512(d+j, v0);
            _mm512_storeu_si512(d+j+1, v1);
            _mm512_storeu_si512(d+j+2, v2);
            _mm512_storeu_si512(d+j+3, v3);
            _mm512_storeu_si512(d+j+4, v4);
            _mm512_storeu_si512(d+j+5, v5);
            _mm512_storeu_si512(d+j+6, v6);
            _mm512_storeu_si512(d+j+7, v7);
            _mm512_storeu_si512(d+j+8, v8);
            _mm512_storeu_si512(d+j+9, v9);
            _mm512_storeu_si512(d+j+10, v10);
            _mm512_storeu_si512(d+j+11, v11);
            _mm512_storeu_si512(d+j+12, v12);
            _mm512_storeu_si512(d+j+13, v13);
            _mm512_storeu_si512(d+j+14, v14);
            _mm512_storeu_si512(d+j+15, v15);
        }
        for (; j < chunks; j++) {
            _mm512_storeu_si512(d+j, _mm512_loadu_si512(s+j));
        }
    }

    // Benchmark
    auto start = high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        *reinterpret_cast<uint64_t*>(buffer) = num_elements;
        const __m512i* s = (const __m512i*)data;
        __m512i* d = (__m512i*)(buffer + 8);

        size_t chunks = data_bytes / 64;
        size_t j = 0;
        for (; j + 16 <= chunks; j += 16) {
            __m512i v0 = _mm512_loadu_si512(s+j);
            __m512i v1 = _mm512_loadu_si512(s+j+1);
            __m512i v2 = _mm512_loadu_si512(s+j+2);
            __m512i v3 = _mm512_loadu_si512(s+j+3);
            __m512i v4 = _mm512_loadu_si512(s+j+4);
            __m512i v5 = _mm512_loadu_si512(s+j+5);
            __m512i v6 = _mm512_loadu_si512(s+j+6);
            __m512i v7 = _mm512_loadu_si512(s+j+7);
            __m512i v8 = _mm512_loadu_si512(s+j+8);
            __m512i v9 = _mm512_loadu_si512(s+j+9);
            __m512i v10 = _mm512_loadu_si512(s+j+10);
            __m512i v11 = _mm512_loadu_si512(s+j+11);
            __m512i v12 = _mm512_loadu_si512(s+j+12);
            __m512i v13 = _mm512_loadu_si512(s+j+13);
            __m512i v14 = _mm512_loadu_si512(s+j+14);
            __m512i v15 = _mm512_loadu_si512(s+j+15);

            _mm512_storeu_si512(d+j, v0);
            _mm512_storeu_si512(d+j+1, v1);
            _mm512_storeu_si512(d+j+2, v2);
            _mm512_storeu_si512(d+j+3, v3);
            _mm512_storeu_si512(d+j+4, v4);
            _mm512_storeu_si512(d+j+5, v5);
            _mm512_storeu_si512(d+j+6, v6);
            _mm512_storeu_si512(d+j+7, v7);
            _mm512_storeu_si512(d+j+8, v8);
            _mm512_storeu_si512(d+j+9, v9);
            _mm512_storeu_si512(d+j+10, v10);
            _mm512_storeu_si512(d+j+11, v11);
            _mm512_storeu_si512(d+j+12, v12);
            _mm512_storeu_si512(d+j+13, v13);
            _mm512_storeu_si512(d+j+14, v14);
            _mm512_storeu_si512(d+j+15, v15);
        }
        for (; j < chunks; j++) {
            _mm512_storeu_si512(d+j, _mm512_loadu_si512(s+j));
        }
    }

    auto end = high_resolution_clock::now();

    double ns_per_op = duration_cast<nanoseconds>(end - start).count() / (double)iterations;

    volatile uint8_t sink = buffer[0];
    (void)sink;

    free(data);
    free(buffer);

    return data_bytes / ns_per_op;
}

int main() {
    unsigned int num_cores = std::thread::hardware_concurrency();

    std::cout << "\n🚀🚀🚀 MULTITHREADED THEORETICAL MAXIMUM 🚀🚀🚀\n";
    std::cout << "System: " << num_cores << " cores detected\n\n";

    struct Test {
        size_t num_elements;
        const char* name;
        size_t iterations;
    };

    std::vector<Test> tests = {
        {128, "1KB", 1000},
        {1024, "8KB", 500},
        {16384, "128KB", 100},
    };

    std::cout << "| Threads | Size | Per-Thread (GB/s) | Aggregate (GB/s) | vs Single-Thread |\n";
    std::cout << "|---------|------|-------------------|------------------|------------------|---\n";

    for (const auto& test : tests) {
        // Single-threaded baseline
        double single_thread_gbps = benchmark_thread(test.num_elements, test.iterations);

        std::cout << "| " << std::setw(7) << 1
                  << " | " << std::setw(4) << test.name
                  << " | " << std::setw(17) << std::fixed << std::setprecision(2) << single_thread_gbps
                  << " | " << std::setw(16) << single_thread_gbps
                  << " | " << std::setw(16) << "1.0x |\n";

        // Multi-threaded tests
        for (unsigned int num_threads : {2u, 4u, 8u, 16u, num_cores}) {
            if (num_threads > num_cores) continue;

            std::vector<std::thread> threads;
            std::vector<double> results(num_threads);

            for (unsigned int i = 0; i < num_threads; ++i) {
                threads.emplace_back([&, i]() {
                    results[i] = benchmark_thread(test.num_elements, test.iterations);
                });
            }

            for (auto& t : threads) {
                t.join();
            }

            double total_gbps = 0;
            for (double r : results) {
                total_gbps += r;
            }
            double per_thread_avg = total_gbps / num_threads;

            std::cout << "| " << std::setw(7) << num_threads
                      << " | " << std::setw(4) << test.name
                      << " | " << std::setw(17) << per_thread_avg
                      << " | " << std::setw(16) << total_gbps
                      << " | " << std::setw(16) << (total_gbps / single_thread_gbps) << "x |\n";
        }

        std::cout << "|---------|------|-------------------|------------------|------------------|---\n";
    }

    std::cout << "\n💡 Aggregate = Per-Thread × Number of Threads\n";
    std::cout << "💡 On " << num_cores << " cores: ~" << std::fixed << std::setprecision(0)
              << (150.0 * num_cores) << " GB/s theoretical maximum\n\n";

    return 0;
}

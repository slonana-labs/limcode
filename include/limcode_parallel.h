/**
 * @file limcode_parallel.h
 * @brief Parallel multi-threaded limcode encoding for batch operations
 *
 * Enables encoding multiple transactions/messages in parallel using
 * lock-free work stealing and SIMD optimizations.
 */

#pragma once

#include "common/limcode.h"
#include <atomic>
#include <functional>
#include <thread>
#include <vector>

namespace slonana {
namespace limcode {

/**
 * @brief Lock-free work-stealing queue for parallel encoding tasks
 */
template <typename T> class WorkStealingQueue {
private:
  alignas(64) std::atomic<size_t> head_{0};
  alignas(64) std::atomic<size_t> tail_{0};
  std::vector<T> buffer_;
  size_t mask_;

public:
  explicit WorkStealingQueue(size_t capacity = 1024)
      : buffer_(capacity), mask_(capacity - 1) {
    // Ensure capacity is power of 2
    if ((capacity & (capacity - 1)) != 0) {
      throw std::invalid_argument("Capacity must be power of 2");
    }
  }

  bool try_push(const T &item) {
    size_t tail = tail_.load(std::memory_order_relaxed);
    size_t next_tail = (tail + 1) & mask_;

    if (next_tail == head_.load(std::memory_order_acquire)) {
      return false; // Queue full
    }

    buffer_[tail] = item;
    tail_.store(next_tail, std::memory_order_release);
    return true;
  }

  bool try_pop(T &item) {
    size_t tail = tail_.load(std::memory_order_relaxed);

    if (tail == head_.load(std::memory_order_acquire)) {
      return false; // Queue empty
    }

    size_t prev_tail = (tail - 1) & mask_;
    item = buffer_[prev_tail];
    tail_.store(prev_tail, std::memory_order_release);
    return true;
  }

  bool try_steal(T &item) {
    size_t head = head_.load(std::memory_order_acquire);
    size_t tail = tail_.load(std::memory_order_acquire);

    if (head == tail) {
      return false; // Empty
    }

    item = buffer_[head];
    head_.store((head + 1) & mask_, std::memory_order_release);
    return true;
  }
};

/**
 * @brief Parallel batch encoder for encoding multiple transactions
 */
class ParallelBatchEncoder {
private:
  size_t num_threads_;
  std::vector<std::thread> workers_;
  std::vector<WorkStealingQueue<std::function<void()>>> queues_;
  std::atomic<bool> stop_{false};
  std::atomic<size_t> active_tasks_{0};

public:
  explicit ParallelBatchEncoder(size_t num_threads = 0)
      : num_threads_(num_threads == 0 ? std::thread::hardware_concurrency()
                                      : num_threads) {

    queues_.reserve(num_threads_);
    for (size_t i = 0; i < num_threads_; ++i) {
      queues_.emplace_back(1024);
    }

    // Spawn worker threads
    for (size_t i = 0; i < num_threads_; ++i) {
      workers_.emplace_back([this, i]() { worker_loop(i); });
    }
  }

  ~ParallelBatchEncoder() {
    stop_.store(true, std::memory_order_release);
    for (auto &worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

  /**
   * @brief Encode multiple transactions in parallel
   *
   * @tparam TxIterator Iterator over VersionedTransaction objects
   * @param begin Start iterator
   * @param end End iterator
   * @return Vector of encoded byte buffers (one per transaction)
   */
  template <typename TxIterator>
  std::vector<std::vector<uint8_t>> encode_batch(TxIterator begin,
                                                 TxIterator end) {
    size_t count = std::distance(begin, end);
    std::vector<std::vector<uint8_t>> results(count);

    // Submit encoding tasks
    active_tasks_.store(count, std::memory_order_release);

    size_t task_id = 0;
    for (auto it = begin; it != end; ++it, ++task_id) {
      size_t queue_idx = task_id % num_threads_;

      queues_[queue_idx].try_push([this, &results, task_id, tx = *it]() {
        LimcodeEncoder encoder;
        encoder.write_versioned_transaction(tx);
        results[task_id] = std::move(encoder.into_vec());
        active_tasks_.fetch_sub(1, std::memory_order_release);
      });
    }

    // Wait for all tasks to complete
    while (active_tasks_.load(std::memory_order_acquire) > 0) {
      std::this_thread::yield();
    }

    return results;
  }

  /**
   * @brief Encode multiple byte buffers in parallel (generic encoding)
   *
   * @param inputs Vector of input data to encode
   * @param encode_fn Encoding function: void(LimcodeEncoder&, const T&)
   * @return Vector of encoded byte buffers
   */
  template <typename T, typename EncodeFn>
  std::vector<std::vector<uint8_t>>
  encode_batch_generic(const std::vector<T> &inputs, EncodeFn &&encode_fn) {

    size_t count = inputs.size();
    std::vector<std::vector<uint8_t>> results(count);

    active_tasks_.store(count, std::memory_order_release);

    for (size_t i = 0; i < count; ++i) {
      size_t queue_idx = i % num_threads_;

      queues_[queue_idx].try_push([this, &results, i, &inputs, &encode_fn]() {
        LimcodeEncoder encoder;
        encode_fn(encoder, inputs[i]);
        results[i] = std::move(encoder.into_vec());
        active_tasks_.fetch_sub(1, std::memory_order_release);
      });
    }

    while (active_tasks_.load(std::memory_order_acquire) > 0) {
      std::this_thread::yield();
    }

    return results;
  }

private:
  void worker_loop(size_t thread_id) {
    std::function<void()> task;

    while (!stop_.load(std::memory_order_acquire)) {
      // Try to pop from own queue
      if (queues_[thread_id].try_pop(task)) {
        task();
        continue;
      }

      // Work stealing: try to steal from other queues
      bool stolen = false;
      for (size_t i = 1; i < num_threads_; ++i) {
        size_t victim = (thread_id + i) % num_threads_;
        if (queues_[victim].try_steal(task)) {
          task();
          stolen = true;
          break;
        }
      }

      if (!stolen) {
        std::this_thread::yield();
      }
    }
  }
};

/**
 * @brief Convenience function: encode multiple transactions in parallel
 */
template <typename TxIterator>
inline std::vector<std::vector<uint8_t>>
encode_transactions_parallel(TxIterator begin, TxIterator end,
                             size_t num_threads = 0) {

  ParallelBatchEncoder encoder(num_threads);
  return encoder.encode_batch(begin, end);
}

/**
 * @brief Parallel chunked copier for mega-blocks (1MB - 48MB Solana blocks)
 *
 * Splits large block into chunks and processes in parallel across cores.
 * Critical for Solana's 48MB maximum block size.
 */
class ParallelMegaBlockCopier {
private:
  size_t num_threads_;
  static constexpr size_t CHUNK_SIZE = 4194304; // 4MB chunks

public:
  explicit ParallelMegaBlockCopier(size_t num_threads = 0)
      : num_threads_(num_threads == 0 ? std::thread::hardware_concurrency()
                                      : num_threads) {}

  /**
   * @brief Copy mega-block (1MB - 48MB) in parallel
   *
   * Splits data into 4MB chunks and processes on multiple cores.
   * Uses non-temporal stores to bypass cache.
   *
   * @param dst Destination buffer (must be pre-allocated!)
   * @param src Source data
   * @param size Total size (1MB to 48MB)
   */
  void copy_parallel(uint8_t *dst, const uint8_t *src, size_t size) {
    if (size < CHUNK_SIZE || num_threads_ == 1) {
      // Small enough or single-threaded: use direct copy
#if defined(__AVX512F__)
      limcode_nt_copy_avx512(dst, src, size);
#elif defined(__AVX2__)
      limcode_nt_copy_avx2(dst, src, size);
#else
      std::memcpy(dst, src, size);
#endif
      return;
    }

    // Split into chunks
    size_t num_chunks = (size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    size_t chunks_per_thread = std::max(size_t(1), num_chunks / num_threads_);

    std::vector<std::thread> workers;
    std::atomic<size_t> chunk_idx{0};

    for (size_t i = 0; i < num_threads_; ++i) {
      workers.emplace_back([&]() {
        while (true) {
          size_t idx = chunk_idx.fetch_add(1, std::memory_order_relaxed);
          if (idx >= num_chunks)
            break;

          size_t offset = idx * CHUNK_SIZE;
          size_t chunk_size = std::min(CHUNK_SIZE, size - offset);

#if defined(__AVX512F__)
          limcode_nt_copy_avx512(dst + offset, src + offset, chunk_size);
#elif defined(__AVX2__)
          limcode_nt_copy_avx2(dst + offset, src + offset, chunk_size);
#else
          std::memcpy(dst + offset, src + offset, chunk_size);
#endif
        }
      });
    }

    for (auto &worker : workers) {
      worker.join();
    }
  }

  /**
   * @brief Encode mega-block (e.g., 48MB Solana block) with parallel chunking
   *
   * Pre-allocates buffer and encodes in parallel chunks
   */
  std::vector<uint8_t> encode_mega_block(const uint8_t *data, size_t size) {
    std::vector<uint8_t> result(size + 1024); // Extra space for header

    LimcodeEncoder encoder;
    encoder.write_u64(size); // Write size prefix

    // Copy data in parallel
    size_t header_size = encoder.size();
    result.resize(header_size + size);
    std::memcpy(result.data(), encoder.data(), header_size);

    copy_parallel(result.data() + header_size, data, size);

    return result;
  }
};

/**
 * @brief Convenience function: Copy 48MB Solana block in parallel
 */
inline void copy_solana_block_parallel(uint8_t *dst, const uint8_t *src,
                                       size_t size) {
  ParallelMegaBlockCopier copier;
  copier.copy_parallel(dst, src, size);
}

} // namespace limcode
} // namespace slonana

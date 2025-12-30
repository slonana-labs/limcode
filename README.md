# Limcode

**High-performance Solana serialization - 5x faster than bincode**

Limcode is a drop-in replacement for bincode/wincode that produces identical wire-compatible output while being significantly faster.

## Benchmark Results

| Block Size | Limcode | Wincode | Bincode | vs Wincode | vs Bincode |
|------------|---------|---------|---------|------------|------------|
| 100 entries | 820,000/s | 160,000/s | 155,000/s | **5.1x** | **5.3x** |
| 500 entries | 142,000/s | 30,000/s | 29,000/s | **4.7x** | **4.9x** |
| 1000 entries | 57,000/s | 14,000/s | 13,500/s | **4.1x** | **4.2x** |
| 2000 entries | 26,000/s | 6,500/s | 6,300/s | **4.0x** | **4.1x** |

Peak throughput: **194 Gbps**

## Installation

Header-only library. Just copy the `include/limcode` directory to your project.

```cmake
add_subdirectory(limcode)
target_link_libraries(your_target PRIVATE limcode)
```

## Usage

```cpp
#include <limcode/limcode.h>

// Serialize entries (zero-copy, returns span to thread-local buffer)
std::vector<limcode::Entry> entries = ...;
auto bytes = limcode::serialize(entries);

// Or get a vector copy
auto vec = limcode::serialize_vec(entries);

// Serialize single entry
auto entry_bytes = limcode::serialize_entry(entry);

// Serialize single transaction
auto tx_bytes = limcode::serialize_transaction(tx);
```

## Wire Format Compatibility

Limcode produces **byte-identical output** to Solana's wincode format:

- Uses ShortVec (1-3 byte varint) for vector lengths
- Little-endian byte order
- Compatible with Rust's solana-sdk serialization

## How It's Fast

1. **Thread-local 16MB buffer** - Zero malloc in hot path
2. **Pointer-passing style** - No position variable overhead
3. **SIMD bulk copies** - AVX2/AVX-512 for 32/64 byte operations
4. **Aggressive prefetching** - Hides memory latency

## Format Comparison

| Format | Vec Length Encoding | Use Case |
|--------|---------------------|----------|
| **Limcode** | ShortVec (1-3 bytes) | Solana transactions (optimized) |
| **Wincode** | ShortVec (1-3 bytes) | Solana transactions (baseline) |
| **Bincode** | u64 (8 bytes) | General Rust serialization |

## Building

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j

# Run tests
ctest --output-on-failure

# Run benchmark
./limcode_benchmark
```

## License

MIT

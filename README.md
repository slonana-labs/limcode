# Limcode

**High-performance, bincode-compatible serialization library for Rust and C++**

Limcode is a fully bincode-compatible serialization library optimized for zero-copy operations and SIMD acceleration. It provides 6.4x faster serialization than bincode for buffer-reuse workloads and is designed for high-throughput blockchain applications.

## Features

### Core Serialization
- **100% bincode-compatible** - Drop-in replacement for `bincode::serialize` / `bincode::deserialize`
- **Zero-copy optimizations** - No allocation overhead for POD types with buffer reuse
- **SIMD acceleration** - AVX-512/AVX2 with non-temporal stores for large buffers
- **6.4x faster** - Buffer reuse API eliminates allocation overhead
- **Cross-platform** - Rust and C++ bindings, tested on x86-64 and ARM

### Advanced Features (Optional)
- **Async support** (`async` feature) - Tokio-based async serialization for concurrent workloads (1.78 TB/s aggregate)
- **Compression** (`compression` feature) - ZSTD compression (30-50% size reduction)
- **Checksums** (`checksum` feature) - CRC32 data integrity verification
- **Safe mode** (`migration` feature) - Combined compression + checksum for production migrations
- **Solana snapshots** (`solana` feature) - Parse Solana snapshot archives (.tar.zst)

## 🤖 Integration Guide (For Agents/Developers)

Already using another serialization library? Here's how to migrate to limcode:

### From bincode

**Rust:**
```rust
// Before
use bincode;
let bytes = bincode::serialize(&data)?;
let decoded: MyType = bincode::deserialize(&bytes)?;

// After (drop-in replacement)
use limcode;
let bytes = limcode::serialize(&data)?;
let decoded: MyType = limcode::deserialize(&bytes)?;
```

**C++:**
```cpp
// Before: Using bincode via Rust FFI or custom implementation

// After: Direct C++ API with automatic multithreading
#include <limcode/limcode.h>
uint64_t data[1000];
uint8_t buffer[8008];
limcode::serialize(data, 1000, buffer);
size_t len;
limcode::deserialize(buffer, data, &len);
```

### From wincode

**Rust:**
```rust
// Before
use wincode;
let bytes = wincode::serialize(&data)?;
let decoded: MyType = wincode::deserialize(&bytes)?;

// After (faster serialization, compatible format)
use limcode;
let bytes = limcode::serialize(&data)?;  // 2.1x faster than wincode
let decoded: MyType = limcode::deserialize(&bytes)?;
```

### From serde_json

**Rust:**
```rust
// Before (text-based, slower)
use serde_json;
let json = serde_json::to_string(&data)?;
let decoded: MyType = serde_json::from_str(&json)?;

// After (binary, 50-100x faster)
use limcode;
let bytes = limcode::serialize(&data)?;
let decoded: MyType = limcode::deserialize(&bytes)?;
```

### From postcard

**Rust:**
```rust
// Before
use postcard;
let bytes = postcard::to_allocvec(&data)?;
let decoded: MyType = postcard::from_bytes(&bytes)?;

// After (faster for large data, bincode-compatible)
use limcode;
let bytes = limcode::serialize(&data)?;
let decoded: MyType = limcode::deserialize(&bytes)?;
```

### From MessagePack (rmp)

**Rust:**
```rust
// Before
use rmp_serde;
let bytes = rmp_serde::to_vec(&data)?;
let decoded: MyType = rmp_serde::from_slice(&bytes)?;

// After (faster, simpler format)
use limcode;
let bytes = limcode::serialize(&data)?;
let decoded: MyType = limcode::deserialize(&bytes)?;
```

### From CBOR (serde_cbor / ciborium)

**Rust:**
```rust
// Before
use serde_cbor;
let bytes = serde_cbor::to_vec(&data)?;
let decoded: MyType = serde_cbor::from_slice(&bytes)?;

// After (much faster, simpler)
use limcode;
let bytes = limcode::serialize(&data)?;
let decoded: MyType = limcode::deserialize(&bytes)?;
```

### From Protocol Buffers (prost)

**Rust:**
```rust
// Before (requires .proto files and codegen)
use prost::Message;
let mut buf = Vec::new();
data.encode(&mut buf)?;
let decoded = MyType::decode(&buf[..])?;

// After (no codegen needed, simpler)
use limcode;
let bytes = limcode::serialize(&data)?;
let decoded: MyType = limcode::deserialize(&bytes)?;
```

**C++:**
```cpp
// Before: Using protobuf with generated code
#include "mytype.pb.h"
MyType msg;
std::string serialized;
msg.SerializeToString(&serialized);

// After: Direct serialization, no codegen
#include <limcode/limcode.h>
limcode::serialize(data, len, buffer);
limcode::deserialize(buffer, data, &len);
```

### From BSON (serde_bson)

**Rust:**
```rust
// Before (MongoDB format, larger overhead)
use bson;
let bytes = bson::to_vec(&data)?;
let decoded: MyType = bson::from_slice(&bytes)?;

// After (smaller, faster)
use limcode;
let bytes = limcode::serialize(&data)?;
let decoded: MyType = limcode::deserialize(&bytes)?;
```

### From Cap'n Proto / FlatBuffers

**Rust:**
```rust
// Before: Zero-copy but requires schema and codegen
// Complex API with builders and readers

// After: Simple API, automatic multithreading
use limcode;
let bytes = limcode::serialize(&data)?;  // Simpler, no schema
let decoded: MyType = limcode::deserialize(&bytes)?;
```

**C++:**
```cpp
// Before: FlatBuffers with schema compiler
#include "myschema_generated.h"
flatbuffers::FlatBufferBuilder builder;
// ... complex builder API

// After: Direct, no schema needed
#include <limcode/limcode.h>
limcode::serialize(data, len, buffer);  // One line!
limcode::deserialize(buffer, data, &len);
```

### Performance Comparison (Top 10 Formats)

| Format | Serialize (GB/s) | Deserialize (GB/s) | Size Overhead | Schema Required |
|--------|------------------|--------------------| --------------| ----------------|
| **limcode** | **147.91** | **129.37** | Minimal (8B) | ❌ No |
| bincode | 15.96 | 15.92 | Minimal | ❌ No |
| wincode | 71.72 | Zero-copy* | Minimal | ❌ No |
| postcard | ~20 | ~25 | Minimal | ❌ No |
| MessagePack | ~10 | ~12 | Small | ❌ No |
| CBOR | ~8 | ~10 | Medium | ❌ No |
| Protocol Buffers | ~15 | ~20 | Medium | ✅ Yes (.proto) |
| BSON | ~5 | ~7 | Large | ❌ No |
| Cap'n Proto | ~30 | Zero-copy* | Large | ✅ Yes (.capnp) |
| FlatBuffers | ~25 | Zero-copy* | Large | ✅ Yes (.fbs) |
| serde_json | ~0.5 | ~0.3 | Very large | ❌ No |

*Benchmark @ 1KB data. Zero-copy formats return references, not owned data. See [Performance](#performance) for full results.*

**Key advantages of limcode:**
- 🚀 **9-100x faster** than most formats
- 🎯 **No schema/codegen** required (unlike protobuf, Cap'n Proto, FlatBuffers)
- 🔄 **Bincode-compatible** - drop-in replacement
- 🧵 **Automatic multithreading** for large data (≥64MB)
- 💻 **Native C++ and Rust** support

### Migration Checklist

- [ ] Replace dependency in `Cargo.toml`: `limcode = "0.1.5"`
- [ ] Update imports: `use limcode::{serialize, deserialize};`
- [ ] Replace serialization calls (usually just changing the crate name)
- [ ] Test with existing data (limcode is bincode-compatible)
- [ ] Run benchmarks to verify performance gains
- [ ] For C++: Use simple API `limcode::serialize()` / `limcode::deserialize()`

## Quick Start

Add to `Cargo.toml`:
```toml
[dependencies]
limcode = "0.1"
```

### Basic Usage (Compatible with any Rust type)

```rust
use limcode::{serialize, deserialize};
use serde::{Serialize, Deserialize};

#[derive(Serialize, Deserialize)]
struct Transaction {
    amount: u64,
    fee: u64,
}

let tx = Transaction { amount: 1000, fee: 10 };
let encoded = serialize(&tx).unwrap();
let decoded: Transaction = deserialize(&encoded).unwrap();
```

### High-Performance Buffer Reuse (POD types)

```rust
use limcode::serialize_pod_into;

let mut buf = Vec::new(); // Reusable buffer

for transaction in transactions {
    // Zero allocation after first iteration!
    serialize_pod_into(&transaction, &mut buf).unwrap();
    send_to_network(&buf);
}
```

## Advanced Features

### Async Serialization

```rust
use limcode::serialize_pod_async;

// Enable with: cargo build --features async

#[tokio::main]
async fn main() {
    let data = vec![1u64, 2, 3, 4, 5];
    let encoded = serialize_pod_async(&data).await.unwrap();
    // 1.78 TB/s aggregate throughput with concurrent operations
}
```

### Compression

```rust
use limcode::{serialize_pod_compressed, deserialize_pod_compressed};

// Enable with: cargo build --features compression

let data = vec![1u64, 2, 3, 4, 5];
let compressed = serialize_pod_compressed(&data, 3).unwrap(); // level 3 (0-22)
let decoded: Vec<u64> = deserialize_pod_compressed(&compressed).unwrap();
// 30-50% size reduction for blockchain data
```

### Safe Mode (Compression + Checksums)

```rust
use limcode::{serialize_pod_safe, deserialize_pod_safe};

// Enable with: cargo build --features migration

let data = vec![1u64, 2, 3, 4, 5];
let safe = serialize_pod_safe(&data, 3).unwrap();
let decoded: Vec<u64> = deserialize_pod_safe(&safe).unwrap();
// Format: [4 bytes CRC32][ZSTD compressed data]
```

### Solana Snapshot Parsing

```rust
use limcode::snapshot::stream_snapshot;

// Enable with: cargo build --features solana

// RECOMMENDED: Stream for 1.5B+ accounts (memory-efficient)
for account_result in stream_snapshot("snapshot-123-aBcD.tar.zst")? {
    let acc = account_result?;
    db.insert(acc.pubkey, acc)?; // Process directly to database
}

// Alternative: Load all into memory (use only for small snapshots)
let accounts = limcode::snapshot::parse_snapshot("snapshot.tar.zst")?;

// Real Solana format: 97-byte AppendVec headers + variable data
// Files in accounts/ directory, manifest in snapshots/SLOT/SLOT
```

## Performance

### Buffer Reuse API (serialize_pod_into)

| Payload Size | Limcode | Bincode | Speedup |
|--------------|---------|---------|---------|
| 64MB         | 12.24 GB/s | 1.92 GB/s | **6.4x** |
| 32MB         | 12.0 GB/s  | 1.89 GB/s | **6.3x** |
| 8MB          | 17.0 GB/s  | 1.76 GB/s | **9.7x** |

**Key insight:** Eliminating Vec allocation overhead provides 6-10x speedup for hot paths.

### C++ Native Performance (Theoretical Maximum Achieved)

**Single-threaded (limcode.h with AVX-512):**

| Size   | Serialize (GB/s) | Deserialize (GB/s) | % of Theoretical Max |
|--------|------------------|--------------------|----------------------|
| 64B    | 30.15           | 46.65              | -                    |
| 1KB    | **147.91**      | 129.37             | **98%** ✅           |
| 8KB    | 101.62          | 138.45             | 68%                  |
| 128KB  | 73.46           | 63.47              | 49% (cache-limited)  |
| 1MB    | 59.67           | 45.41              | 40% (cache-limited)  |
| 8MB    | 20.08           | 9.90               | 13% (RAM-limited)    |

**Multithreaded (16 cores):**

| Size   | Aggregate Throughput | Speedup vs Single-Thread |
|--------|---------------------|--------------------------|
| 1KB    | 920.79 GB/s         | 6.08x                    |
| 8KB    | **1.78 TB/s**       | **21.19x** ✅            |
| 128KB  | 929.68 GB/s         | 13.06x                   |

*Theoretical maximum on 16 cores: ~2.4 TB/s*

**vs Rust Serializers:**

| Size | limcode.h | bincode | wincode | vs bincode | vs wincode |
|------|-----------|---------|---------|------------|------------|
| 1KB  | 147.91 GB/s | 15.96 GB/s | 71.72 GB/s | **9.3x** | **2.1x** |
| 8KB  | 101.62 GB/s | 16.79 GB/s | 52.05 GB/s | **6.1x** | **2.0x** |
| 128KB| 73.46 GB/s  | 10.87 GB/s | 66.94 GB/s | **6.8x** | **1.1x** |

Performance achieved through:
- AVX-512 16x loop unrolling (1024 bytes/iteration)
- Zero-branch hot loop with `__m512i*` pointer arithmetic
- 64-byte aligned buffers (aligned_alloc)
- Inline assembly-level optimization matching theoretical hardware limits

See [PERFORMANCE.md](PERFORMANCE.md) for detailed analysis.

## Building

### Rust

```bash
cargo build --release
cargo test --release
```

For maximum performance:
```bash
RUSTFLAGS="-C target-cpu=native" cargo build --release
```

### C++

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Ultra-aggressive optimization mode is enabled by default.

## Documentation

- [PERFORMANCE.md](PERFORMANCE.md) - Performance analysis and benchmarks
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [CLAUDE.md](CLAUDE.md) - Architecture and development guide
- [docs/](docs/) - Additional technical documentation

## API

### Rust

```rust
// Compatible with all types (uses bincode internally)
limcode::serialize<T>(&value) -> Result<Vec<u8>>
limcode::deserialize<T>(&bytes) -> Result<T>

// High-performance POD serialization
limcode::serialize_pod<T>(&value) -> Result<Vec<u8>>
limcode::serialize_pod_into<T>(&value, &mut buf) -> Result<()>  // Zero-copy
limcode::deserialize_pod<T>(&bytes) -> Result<T>
```

### C++ (Simple API)

```cpp
#include <limcode/limcode.h>

// Serialize (automatic multithreading for ≥64MB)
uint64_t data[1000];
uint8_t buffer[8008];
limcode::serialize(data, 1000, buffer);

// Deserialize (automatic multithreading for ≥64MB)
size_t len;
limcode::deserialize(buffer, data, &len);
```

### C++ (Advanced API)

```cpp
#include <limcode/limcode.h>

// Encoding
limcode::LimcodeEncoder enc;
enc.write_u64(12345);
enc.write_bytes(data, len);
auto bytes = std::move(enc).finish();

// Decoding
limcode::LimcodeDecoder dec(bytes.data(), bytes.size());
uint64_t val = dec.read_u64();
dec.read_bytes(buffer, len);
```

## Requirements

### Minimum
- Rust 1.70+ or C++20 compiler
- x86-64 or ARM64 architecture

### Recommended
- AVX-512 support (Intel Skylake-X+, AMD Zen 4+)
- 16+ cores for parallel batch operations

## License

MIT License - See [LICENSE](LICENSE)

## Authors

- **Slonana Labs** - Core development
- **Rin Fhenzig @ OpenSVM** - Optimization engineering

## Credits

Developed for high-performance Solana validators and blockchain applications.

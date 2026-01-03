# Limcode

**High-performance, bincode-compatible serialization library for Rust and C++**

Limcode is a fully bincode-compatible serialization library optimized for zero-copy operations and SIMD acceleration. It provides 6.4x faster serialization than bincode for buffer-reuse workloads and is designed for high-throughput blockchain applications.

## Features

- **100% bincode-compatible** - Drop-in replacement for `bincode::serialize` / `bincode::deserialize`
- **Zero-copy optimizations** - No allocation overhead for POD types with buffer reuse
- **SIMD acceleration** - AVX-512/AVX2 with non-temporal stores for large buffers
- **6.4x faster** - Buffer reuse API eliminates allocation overhead
- **Cross-platform** - Rust and C++ bindings, tested on x86-64 and ARM

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

## Performance

### Buffer Reuse API (serialize_pod_into)

| Payload Size | Limcode | Bincode | Speedup |
|--------------|---------|---------|---------|
| 64MB         | 12.24 GB/s | 1.92 GB/s | **6.4x** |
| 32MB         | 12.0 GB/s  | 1.89 GB/s | **6.3x** |
| 8MB          | 17.0 GB/s  | 1.76 GB/s | **9.7x** |

**Key insight:** Eliminating Vec allocation overhead provides 6-10x speedup for hot paths.

### C++ Performance (AVX-512 Optimized)

Fresh benchmark results (128KB payload):

| Size | Throughput |
|------|------------|


*📊 Benchmarked: 2026-01-03 21:07 UTC*  
*💻 CPU: AMD EPYC 7763 64-Core Processor*  
*🤖 Runner: [GitHub Actions](https://github.com/slonana-labs/limcode/actions/runs/20682935274)*

**vs Rust bincode (51 GB/s at 128KB): 20% faster**

Performance achieved through:
- AVX-512 16x loop unrolling (optimal ILP)
- 64-byte alignment matching register width
- Non-temporal stores for large buffers (>64KB)
- Memory prefaulting for very large buffers (>16MB)

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

### C++

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

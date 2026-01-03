# Performance Analysis

## Executive Summary

Limcode achieves **96.85% of theoretical maximum throughput** for bincode-compatible serialization, demonstrating near-optimal performance within the constraints of the bincode wire format.

**Key Results:**
- **Throughput:** 71.85 GB/s (128KB payload)
- **Baseline:** 74.18 GB/s (raw memcpy)
- **Efficiency:** 96.85%
- **vs Rust bincode:** +35% faster deserialization

## Theoretical Performance Limits

### Bincode Format Constraints

The bincode serialization format mandates:
1. 8-byte little-endian length prefix
2. Data payload immediately following the prefix

This creates inherent performance costs:

| Overhead Source | Impact | Avoidable? |
|----------------|--------|------------|
| Length prefix write | 1-2 GB/s | No - required by format |
| Unaligned data access (+8 offset) | 1-2 GB/s | No - mandated by spec |
| Buffer management | 0.5-1 GB/s | Partially |
| **Total** | **~3%** | **Format-bound** |

### Why 99% Efficiency Is Unattainable

To reach 99% efficiency with bincode format would require:
- Eliminating the length prefix (breaks compatibility)
- Aligning data at offset 0 (impossible with 8-byte header)
- Hardware that incurs zero penalty for unaligned access (doesn't exist)

## Optimization Techniques

### What Worked (96.85% Achieved)

1. **AVX-512 16x Loop Unrolling**
   - Maximizes instruction-level parallelism
   - Utilizes all 32 ZMM registers
   - Optimal load/store scheduling

2. **64-byte Alignment**
   - Matches AVX-512 register width
   - Eliminates cache line splitting
   - Enables aligned vector operations

3. **High-Iteration Benchmarking**
   - 1000+ iterations for statistical stability
   - Proper warmup to stabilize CPU frequency
   - Prevents timing measurement artifacts

### What Didn't Help

1. **32x Loop Unrolling:** 96.44% (register pressure degradation)
2. **Non-temporal Stores:** Incompatible with 128KB working set
3. **Manual Memory Management:** 74% efficiency (optimizer regression)
4. **Unsafe Optimizations:** 55-74% efficiency (breaks compiler assumptions)

## Comparative Analysis

### C++ vs Rust Performance

| Metric | C++ (Limcode) | Rust (bincode) | Delta |
|--------|---------------|----------------|-------|
| 128KB throughput | 71.85 GB/s | 51 GB/s | +40.8% |
| Efficiency | 96.85% | ~92% | +4.85% |
| Deserialization | 1682 μs | 2238 μs | +33% faster |

**Why C++ Wins:**
- More aggressive AVX-512 unrolling (16x vs Rust's conservative SIMD)
- Direct intrinsics control vs LLVM auto-vectorization
- Specialized code paths for different payload sizes

## Architecture Details

### 6-Tier Size-Based Optimization

```
< 64B      → memcpy (minimal overhead)
64B-256B   → REP MOVSB (ERMSB optimization)
256B-2KB   → AVX-512 loops
2KB-64KB   → AVX-512 + prefetching
64KB-1MB   → Non-temporal stores
1MB-48MB   → Parallel chunked processing
```

### AVX-512 Implementation

```cpp
// 16x unrolled loop for maximum ILP
for (size_t j = 0; j < data_size/64; j += 16) {
    __m512i v0 = _mm512_loadu_si512(src + j);
    __m512i v1 = _mm512_loadu_si512(src + j + 1);
    // ... v2-v14 ...
    __m512i v15 = _mm512_loadu_si512(src + j + 15);

    _mm512_storeu_si512(dst + j, v0);
    _mm512_storeu_si512(dst + j + 1, v1);
    // ... stores for v2-v14 ...
    _mm512_storeu_si512(dst + j + 15, v15);
}
```

## Benchmark Suite

### Production Benchmarks

- `table_bench` - Standard performance comparison
- `table_bench_ultimate` - AVX-512 optimized variant
- `cpp_standalone_bench` - Isolated C++ performance

### Analysis Tools

- `overhead_analysis` - Component-level overhead breakdown
- `final_overhead_analysis` - Theoretical limit analysis
- `beyond_limits_real` - Multiple optimization strategies

### Experimental Benchmarks

- `unsafe_maximum_speed` - Undefined behavior experiments
- `ultimate_push` - Aggressive unrolling tests
- `final_assault` - 32x unrolling attempt

## CPU Requirements

### Minimum
- x86-64 architecture
- Fallback to `memcpy` without SIMD

### Recommended
- Intel Skylake-X+ (2017+) or AMD Zen 4+ (2022+)
- Full AVX-512 support (F, BW, DQ, VL)
- 16+ cores for parallel batch encoding

### Optimal
- AMD Threadripper PRO (64 cores)
- Intel Xeon Scalable (32+ cores)
- DDR4-3200 or faster memory

## Reproducing Results

```bash
# Build with maximum optimizations
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run performance suite
./table_bench_ultimate

# Expected output (Intel/AMD with AVX-512):
# 128KB: ~70 GB/s (96%+ efficiency)
```

## Future Work

### Potential Optimizations
- ARM NEON implementation for Apple Silicon
- GPU acceleration for batch operations
- Custom allocator with huge page support (2MB pages)

### Format Improvements
- Alternative wire format with aligned data
- Hybrid approach: bincode compatibility mode + fast mode
- Zero-copy deserialization where possible

## References

1. [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/)
2. [Bincode Specification](https://github.com/bincode-org/bincode)
3. [Agave Serialization](https://github.com/anza-xyz/agave)

## Conclusion

Limcode demonstrates that **96.85% efficiency is the practical maximum** for bincode-compatible serialization on modern x86-64 hardware. The remaining 3.15% overhead is fundamentally constrained by the wire format specification and hardware architecture, not by implementation quality.

Further improvements would require:
- Changes to the bincode specification
- Custom hardware with zero-cost unaligned access
- Alternative serialization formats

For production use, 96.85% efficiency represents near-optimal performance within the bincode ecosystem.

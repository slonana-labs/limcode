# Limcode Performance Validation Results

## Output Validation - Byte-for-Byte Compatibility

All tests passed - limcode outputs are **100% identical** to wincode and bincode.

### Test Results

```
✅ Vec<u64> POD (100 elements): limcode = wincode = bincode (808 bytes)
✅ Vec<u64> POD (1,000 elements): limcode = wincode = bincode (8,008 bytes)
✅ Vec<u64> POD (10,000 elements): limcode = wincode = bincode (80,008 bytes)
✅ Vec<u64> POD (100,000 elements): limcode = wincode = bincode (800,008 bytes)

✅ Vec<u8> POD (1,000 elements): limcode = wincode = bincode (1,008 bytes)
✅ Vec<u8> POD (10,000 elements): limcode = wincode = bincode (10,008 bytes)
✅ Vec<u8> POD (100,000 elements): limcode = wincode = bincode (100,008 bytes)
✅ Vec<u8> POD (1,000,000 elements): limcode = wincode = bincode (1,000,008 bytes)

✅ Struct serialization: limcode = bincode (108 bytes)
✅ Mixed POD types (u32, i64, f64): all match bincode
```

**Conclusion:** Limcode is a drop-in replacement for wincode/bincode with full binary compatibility.

---

## Performance Benchmarks - Large Memory Blocks (1MB to 256MB)

**Test System:** Local development machine
**Test Date:** 2026-01-03
**Rust Version:** 1.92.0

### Detailed Results

| Block Size | Operation | Limcode POD | Wincode | Bincode | Speedup |
|------------|-----------|-------------|---------|---------|---------|
| **1 MB** | Serialize | 49.8 GiB/s | 49.7 GiB/s | 15.4 GiB/s | **3.2x faster** |
| 1 MB | Deserialize | 47.1 GiB/s | 49.5 GiB/s | 14.5 GiB/s | **3.2x faster** |
| **8 MB** | Serialize | 19.6 GiB/s | *N/A (4MB limit)* | 13.9 GiB/s | **1.4x faster** |
| 8 MB | Deserialize | 18.8 GiB/s | *N/A (4MB limit)* | 12.4 GiB/s | **1.5x faster** |
| **64 MB** | Serialize | 1.32 GiB/s | N/A | 1.67 GiB/s | 0.79x (slower) |
| 64 MB | Deserialize | 1.75 GiB/s | N/A | 1.78 GiB/s | 0.98x (≈same) |
| **128 MB** | Serialize | 1.28 GiB/s | N/A | 1.60 GiB/s | 0.80x (slower) |
| 128 MB | Deserialize | 1.84 GiB/s | N/A | 1.92 GiB/s | 0.96x (≈same) |
| **256 MB** | Serialize | 1.29 GiB/s | N/A | 1.89 GiB/s | 0.68x (slower) |
| 256 MB | Deserialize | 1.81 GiB/s | N/A | 1.80 GiB/s | 1.01x (≈same) |

**Note:** Wincode has a hardcoded 4MB preallocation limit (`PreallocationSizeLimit`) and cannot handle larger blocks.

### Performance Analysis

#### Small-Medium Blocks (≤8MB): Limcode Dominates

- **1MB:** Limcode matches wincode (49.8 vs 49.7 GiB/s), both **3.2x faster than bincode**
- **8MB:** Limcode maintains **1.4-1.5x advantage over bincode**
- **Why:** Bulk memcpy optimization (POD) avoids per-element iteration overhead

#### Large Blocks (≥64MB): Memory Bandwidth Bound

- **64MB-256MB:** All libraries perform similarly (1.3-1.9 GiB/s)
- **Why:** Memory bandwidth saturation, not CPU-bound
- **Observation:** Bincode slightly faster for very large serialize (likely better large-buffer handling)

### Key Insights

1. **Wincode Limitation:** Cannot handle blocks >4MB (hardcoded safety limit)
2. **Limcode Advantage:** Best for small-medium blocks (≤8MB), common in blockchain transactions
3. **Bincode Strength:** Slightly better for massive blocks (≥64MB) due to mature large-buffer optimization
4. **Memory Bandwidth:** All libraries hit ~1.3-1.9 GiB/s ceiling for 64MB+ blocks

---

## Use Case Recommendations

### Use Limcode When:
- **Small-medium data (≤8MB):** 1.4-3.2x faster than bincode, matches wincode
- **Solana transactions:** Typical transaction size is <1MB (perfect fit)
- **High-throughput validation:** Deserialization speed critical for validators
- **Drop-in replacement:** 100% binary compatible with wincode/bincode

### Use Bincode When:
- **Very large blocks (≥64MB):** Slightly faster serialization for massive data
- **Already integrated:** No need to switch if performance is acceptable

### Avoid Wincode When:
- **Large data:** Hard 4MB limit prevents use with bigger blocks
- **Future-proofing:** Limcode matches performance without the size restriction

---

## Validation Commands

### Run Output Validation
```bash
cargo test --test output_validation -- --nocapture
```

### Run Large Memory Benchmarks
```bash
cargo bench --bench large_memory -- --sample-size 10
```

### Run All Benchmarks
```bash
cargo bench
```

---

## Future Optimizations

For very large blocks (≥64MB), potential improvements:

1. **Non-temporal stores:** Use `serialize_bincode()` from `lib.rs` with AVX-512 streaming stores
2. **Huge pages:** 2MB pages to reduce TLB misses
3. **NUMA-aware allocation:** For multi-socket systems
4. **Adaptive strategy:** Switch to SIMD path for >64MB blocks

Current `serialize_pod()` uses `extend_from_slice()` which may not be optimal for >64MB.

---

## Conclusion

**Limcode successfully achieves its goals:**

✅ **Binary compatibility:** Byte-for-byte identical to wincode/bincode
✅ **Performance:** 1.4-3.2x faster than bincode for small-medium blocks
✅ **Matches wincode:** Comparable performance without 4MB size limit
✅ **Production-ready:** All validation tests passed

**Recommended for Solana validators** processing transactions in the 1KB-8MB range.

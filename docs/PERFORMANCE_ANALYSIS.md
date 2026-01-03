# Limcode Performance Analysis

## Current Status

### What We've Achieved ✅

1. **Full Bincode Compatibility**
   - Custom serde Serializer/Deserializer implementation
   - 100% format-compatible with bincode (byte-for-byte identical)
   - Supports ALL serde types (structs, enums, Vec, String, primitives, Option, etc.)
   - Same interface as wincode: `serialize(&T)` and `deserialize<T>(&[u8])`

2. **Test Coverage**
   - All compatibility tests pass
   - Complex nested structs work
   - Enum variants work
   - Large data (1MB+) works

### Current Performance vs Bincode ❌

**We are SLOWER than bincode across the board:**

| Benchmark | Bincode | Limcode | Winner |
|-----------|---------|---------|--------|
| complex_serialize | 54ns | 88ns | Bincode 1.64x faster |
| complex_deserialize | 98ns | 114ns | Bincode 1.16x faster |
| vec_100kb_serialize | 48µs | 48.7µs | ~Equal |
| vec_100kb_deserialize | 71.9µs | 92.3µs | Bincode 1.28x faster |
| string_deserialize | 78ns | 82ns | Bincode 1.05x faster |
| vec_u64_deserialize | 502ns | 597ns | Bincode 1.19x faster |

## Why We're Slower

### Problem 1: No SIMD in Serde Implementation

Our `rust/src/serializer.rs` and `rust/src/deserializer.rs` have SIMD code for `memcpy`, but:

1. **Limited SIMD usage**: Only `fast_copy()` uses AVX2, and only for buffers >64 bytes
2. **Not enabled by default**: Requires `target_feature = "avx2"` which isn't set
3. **Basic operations**: Most serialization is just `extend_from_slice()` (no SIMD)

### Problem 2: Bincode is Highly Optimized

Bincode has been optimized over years:
- Aggressive compiler inlining
- Minimal allocations
- Tight inner loops
- Specialized code paths for common types

### Problem 3: We're Not Using C++ SIMD Encoder

We have ultra-fast C++ code (`limcode.h` with AVX-512) but our serde implementation doesn't use it!

```
Rust serde -> FastWriter -> extend_from_slice (no SIMD)
              ❌ NOT using -> C++ LimcodeEncoder (AVX-512)
```

## Path Forward: How to Actually Beat Bincode

### Strategy 1: Integrate C++ Encoder into Serde (Recommended)

Modify `serializer.rs` to use the C++ encoder:

```rust
pub struct Serializer {
    inner: crate::Encoder,  // Use C++ FFI encoder
}

impl ser::Serializer for &mut Serializer {
    fn serialize_u64(self, v: u64) -> Result<(), Error> {
        self.inner.write_u64(v);  // AVX-512 optimized
        Ok(())
    }

    fn serialize_bytes(self, v: &[u8]) -> Result<(), Error> {
        self.inner.write_bytes(v);  // AVX-512 memcpy
        Ok(())
    }
}
```

**Expected speedup**: 1.5-2x faster than bincode (leveraging existing SIMD)

### Strategy 2: Pure Rust SIMD (No C++ FFI)

Use `std::simd` (nightly) or inline assembly:

```rust
#[cfg(target_feature = "avx2")]
unsafe fn simd_serialize_u64_array(dst: *mut u8, src: &[u64]) {
    use core::arch::x86_64::*;
    // Vectorized writes
}
```

**Expected speedup**: 1.3-1.8x faster than bincode

### Strategy 3: Specialization + Inlining

Add `#[inline(always)]` everywhere and use const generics:

```rust
#[inline(always)]
fn serialize_array<const N: usize>(arr: &[T; N]) -> Result<Vec<u8>, Error> {
    // Specialized for each array size
}
```

**Expected speedup**: 1.1-1.3x faster than bincode

### Strategy 4: Hybrid Approach (Best)

1. Use C++ encoder for large buffers (>4KB) - AVX-512
2. Use specialized Rust for small types (<64 bytes) - inline fast path
3. Use SIMD for arrays and vectors

**Expected speedup**: 2-3x faster than bincode for large data, ~equal for small

## Immediate Next Steps

1. **Fix SIMD integration**: Make serializer use C++ encoder
2. **Enable AVX2**: Build with `-C target-feature=+avx2`
3. **Add specialization**: Fast path for Vec<u8>, Vec<u64>, etc.
4. **Benchmark again**: Verify we beat bincode

## Current Value Proposition

Even though we're slower than bincode right now, we provide:

1. ✅ **Bincode compatibility** - drop-in replacement
2. ✅ **Same interface as wincode** - `serialize`/`deserialize`
3. ✅ **Extensible** - can add custom optimizations
4. ✅ **C++ FFI ready** - can leverage AVX-512 encoder

## Conclusion

**We've achieved the compatibility goal but NOT the performance goal.**

To actually "beat wincode/bincode", we need to integrate the C++ SIMD encoder into the serde implementation. The infrastructure is there, we just need to wire it up correctly.

Current focus should be:
1. Integrate C++ encoder into `serializer.rs`
2. Add SIMD specialization for common types
3. Re-benchmark and verify speedup

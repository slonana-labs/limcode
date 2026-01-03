//! Measure raw memory bandwidth ceiling to understand serialization limits

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};

fn bench_raw_memcpy(c: &mut Criterion) {
    // Test what the actual memory bandwidth ceiling is
    let sizes_mb = [64, 128, 256];

    for size_mb in sizes_mb {
        let num_bytes = size_mb * 1024 * 1024;
        let src: Vec<u8> = vec![0xFF; num_bytes];
        let mut dst: Vec<u8> = vec![0x00; num_bytes];

        let mut group = c.benchmark_group(format!("raw_memcpy_{}MB", size_mb));
        group.throughput(Throughput::Bytes(num_bytes as u64));
        group.sample_size(10);

        // Pure memcpy (no serialization overhead)
        group.bench_function("memcpy", |b| {
            b.iter(|| {
                unsafe {
                    std::ptr::copy_nonoverlapping(src.as_ptr(), dst.as_mut_ptr(), num_bytes);
                }
                black_box(&dst);
            })
        });

        // AVX-512 non-temporal memcpy
        group.bench_function("nt_memcpy", |b| {
            b.iter(|| {
                unsafe {
                    fast_nt_memcpy(dst.as_mut_ptr(), src.as_ptr(), num_bytes);
                }
                black_box(&dst);
            })
        });

        group.finish();
    }
}

/// Non-temporal memory copy using AVX-512
#[inline(always)]
#[allow(unused_mut)] // Parameters may not be mutated on all platforms
#[cfg(target_arch = "x86_64")]
unsafe fn fast_nt_memcpy(mut dst: *mut u8, mut src: *const u8, mut len: usize) {
    #[cfg(target_feature = "avx512f")]
    {
        use core::arch::x86_64::*;

        // Align to 64-byte boundary
        while (dst as usize) & 63 != 0 && len >= 64 {
            std::ptr::copy_nonoverlapping(src, dst, 64);
            src = src.add(64);
            dst = dst.add(64);
            len -= 64;
        }

        // Process 256-byte chunks (4x AVX-512 stores per iteration)
        while len >= 256 {
            let zmm0 = _mm512_loadu_si512(src as *const _);
            let zmm1 = _mm512_loadu_si512(src.add(64) as *const _);
            let zmm2 = _mm512_loadu_si512(src.add(128) as *const _);
            let zmm3 = _mm512_loadu_si512(src.add(192) as *const _);

            _mm512_stream_si512(dst as *mut _, zmm0);
            _mm512_stream_si512(dst.add(64) as *mut _, zmm1);
            _mm512_stream_si512(dst.add(128) as *mut _, zmm2);
            _mm512_stream_si512(dst.add(192) as *mut _, zmm3);

            src = src.add(256);
            dst = dst.add(256);
            len -= 256;
        }

        _mm_sfence();
    }

    // Handle remaining bytes
    if len > 0 {
        std::ptr::copy_nonoverlapping(src, dst, len);
    }
}

#[cfg(not(target_arch = "x86_64"))]
unsafe fn fast_nt_memcpy(dst: *mut u8, src: *const u8, len: usize) {
    std::ptr::copy_nonoverlapping(src, dst, len);
}

criterion_group!(benches, bench_raw_memcpy);
criterion_main!(benches);

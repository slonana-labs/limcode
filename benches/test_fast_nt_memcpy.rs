//! Direct test of fast_nt_memcpy vs standard memcpy

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};

fn bench_memcpy_direct(c: &mut Criterion) {
    let size_mb = 64;
    let num_bytes: usize = size_mb * 1024 * 1024;
    let src: Vec<u8> = vec![0xFF; num_bytes];
    let mut dst: Vec<u8> = vec![0x00; num_bytes];

    let mut group = c.benchmark_group("memcpy_64MB_direct");
    group.throughput(Throughput::Bytes(num_bytes as u64));
    group.sample_size(10);

    // Standard copy_nonoverlapping
    group.bench_function("std_copy", |b| {
        b.iter(|| {
            unsafe {
                std::ptr::copy_nonoverlapping(src.as_ptr(), dst.as_mut_ptr(), num_bytes);
            }
            black_box(&dst);
        })
    });

    // Call limcode's serialize_pod (should use fast_nt_memcpy internally for >64KB)
    group.bench_function("limcode_serialize_pod", |b| {
        let data: Vec<u64> = vec![0xFF; num_bytes / 8];
        b.iter(|| {
            let result = limcode::serialize_pod(black_box(&data)).unwrap();
            black_box(result);
        })
    });

    group.finish();
}

criterion_group!(benches, bench_memcpy_direct);
criterion_main!(benches);

//! Isolate serialization overhead components

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};

fn bench_serialize_components(c: &mut Criterion) {
    let size_mb = 64;
    let num_elements: usize = (size_mb * 1024 * 1024) / 8; // u64 = 8 bytes
    let data: Vec<u64> = (0..num_elements as u64).collect();

    let mut group = c.benchmark_group("serialize_64MB_breakdown");
    group.throughput(Throughput::Bytes((num_elements * 8) as u64));
    group.sample_size(10);

    // Component 1: Vec allocation only
    group.bench_function("1_alloc_only", |b| {
        let total_len: usize = 8 + num_elements * 8;
        b.iter(|| {
            let v: Vec<u8> = Vec::with_capacity(total_len);
            black_box(v);
        })
    });

    // Component 2: Allocation + length prefix write
    group.bench_function("2_alloc_plus_len", |b| {
        let total_len: usize = 8 + num_elements * 8;
        b.iter(|| {
            let mut result = Vec::with_capacity(total_len);
            unsafe {
                let ptr: *mut u8 = result.as_mut_ptr();
                std::ptr::write_unaligned(ptr.cast::<u64>(), (num_elements as u64).to_le());
                result.set_len(8);
            }
            black_box(result);
        })
    });

    // Component 3: Full serialization (current implementation)
    group.bench_function("3_full_serialize", |b| {
        b.iter(|| {
            let result = limcode::serialize_pod(black_box(&data)).unwrap();
            black_box(result);
        })
    });

    // Component 4: Serialize but keep result (no dealloc in loop)
    group.bench_function("4_serialize_reuse", |b| {
        let mut result = Vec::new();
        b.iter(|| {
            result = limcode::serialize_pod(black_box(&data)).unwrap();
            black_box(&result);
        })
    });

    // Component 5: Preallocated buffer (no allocation overhead)
    group.bench_function("5_preallocated", |b| {
        let total_len: usize = 8 + num_elements * 8;
        let mut result: Vec<u8> = Vec::with_capacity(total_len);

        b.iter(|| {
            unsafe {
                result.set_len(0);
                let ptr: *mut u8 = result.as_mut_ptr();

                // Write length prefix
                std::ptr::write_unaligned(ptr.cast::<u64>(), (num_elements as u64).to_le());

                // Copy data
                let src = data.as_ptr() as *const u8;
                std::ptr::copy_nonoverlapping(src, ptr.add(8), num_elements * 8);

                result.set_len(total_len);
            }
            black_box(&result);
        })
    });

    group.finish();
}

criterion_group!(benches, bench_serialize_components);
criterion_main!(benches);

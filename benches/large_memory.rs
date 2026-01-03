//! Benchmark: Large memory blocks up to 256MB
//!
//! Tests POD serialization performance on very large buffers

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};

fn bench_large_memory(c: &mut Criterion) {
    // Test sizes: 1MB, 8MB, 64MB, 128MB, 256MB
    let sizes_mb = [1, 8, 64, 128, 256];

    for size_mb in sizes_mb {
        let num_elements = (size_mb * 1024 * 1024) / 8; // u64 = 8 bytes
        let data: Vec<u64> = (0..num_elements).collect();

        let mut group = c.benchmark_group(format!("memory_{}MB", size_mb));
        group.throughput(Throughput::Bytes((num_elements * 8) as u64));
        group.sample_size(10); // Fewer samples for large data

        println!("\n=== Testing {} MB ({} elements) ===", size_mb, num_elements);

        // Limcode POD
        group.bench_function("limcode_pod_ser", |b| {
            b.iter(|| limcode::serialize_pod(black_box(&data)).unwrap())
        });

        group.bench_function("limcode_pod_de", |b| {
            let bytes = limcode::serialize_pod(&data).unwrap();
            b.iter(|| limcode::deserialize_pod::<u64>(black_box(&bytes)).unwrap())
        });

        // Wincode (skip for >4MB - PreallocationSizeLimit)
        if size_mb <= 4 {
            group.bench_function("wincode_ser", |b| {
                b.iter(|| wincode::serialize(black_box(&data)).unwrap())
            });

            group.bench_function("wincode_de", |b| {
                let bytes = wincode::serialize(&data).unwrap();
                b.iter(|| wincode::deserialize::<Vec<u64>>(black_box(&bytes)).unwrap())
            });
        }

        // Bincode
        group.bench_function("bincode_ser", |b| {
            b.iter(|| bincode::serialize(black_box(&data)).unwrap())
        });

        group.bench_function("bincode_de", |b| {
            let bytes = bincode::serialize(&data).unwrap();
            b.iter(|| bincode::deserialize::<Vec<u64>>(black_box(&bytes)).unwrap())
        });

        group.finish();
    }
}

criterion_group!(benches, bench_large_memory);
criterion_main!(benches);

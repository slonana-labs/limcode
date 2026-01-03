//! Benchmark: Massive scale parallel serialization
//!
//! This benchmark is designed for high-core-count servers (100+ cores)
//! Run with: RAYON_NUM_THREADS=<num_cores> cargo bench --bench massive_scale

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};

fn bench_parallel_massive(c: &mut Criterion) {
    // Test sizes optimized for 240-core machines
    let sizes = [
        1_000_000,      // 1M elements - threshold for parallelism
        10_000_000,     // 10M elements - 100 chunks @ 100K each
        100_000_000,    // 100M elements - 1000 chunks
        1_000_000_000,  // 1B elements - 10K chunks (svm.run scale!)
    ];

    for size in sizes {
        let data: Vec<u64> = (0..size).collect();
        let size_mb = (size * 8) / 1_000_000;

        let mut group = c.benchmark_group(format!("massive_{}M_elements", size / 1_000_000));
        group.throughput(Throughput::Bytes((size * 8) as u64));
        group.sample_size(10);  // Fewer samples for huge datasets

        // Single-threaded baseline
        group.bench_function("serial", |b| {
            b.iter(|| limcode::serialize_pod(black_box(&data)).unwrap())
        });

        // Parallel (uses all available cores)
        group.bench_function("parallel", |b| {
            b.iter(|| limcode::serialize_pod_parallel(black_box(&data)).unwrap())
        });

        group.finish();

        println!("\n=== {} million elements ({} MB) ===", size / 1_000_000, size_mb);
        println!("Cores available: {}", rayon::current_num_threads());
    }
}

criterion_group!(benches, bench_parallel_massive);
criterion_main!(benches);

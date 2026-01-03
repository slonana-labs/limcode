//! Benchmark: limcode vs wincode vs bincode
//!
//! Run with: cargo bench

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};

// Large data benchmark
fn bench_large_vec(c: &mut Criterion) {
    // 1MB of data
    let data: Vec<u8> = (0..1_000_000).map(|i| (i % 256) as u8).collect();
    let bytes = bincode::serialize(&data).unwrap();

    let mut group = c.benchmark_group("large_1mb");
    group.throughput(Throughput::Bytes(bytes.len() as u64));

    group.bench_function("limcode_pod_serialize", |b| {
        b.iter(|| limcode::serialize_pod(black_box(&data)))
    });

    group.bench_function("limcode_serialize", |b| {
        b.iter(|| limcode::serialize(black_box(&data)))
    });

    group.bench_function("wincode_serialize", |b| {
        b.iter(|| wincode::serialize(black_box(&data)))
    });

    group.bench_function("limcode_pod_deserialize", |b| {
        b.iter(|| limcode::deserialize_pod::<u8>(black_box(&bytes)))
    });

    group.bench_function("limcode_deserialize", |b| {
        b.iter(|| limcode::deserialize::<Vec<u8>>(black_box(&bytes)))
    });

    group.bench_function("wincode_deserialize", |b| {
        b.iter(|| wincode::deserialize::<Vec<u8>>(black_box(&bytes)))
    });

    group.finish();
}

// POD benchmark - Vec<u64>
fn bench_pod_vec_u64(c: &mut Criterion) {
    let sizes = [1000, 10000, 100000];

    for size in sizes {
        let data: Vec<u64> = (0..size).collect();

        let mut group = c.benchmark_group(format!("pod_u64_{}", size));
        group.throughput(Throughput::Elements(size));

        // Serialize benchmarks
        group.bench_function("limcode_pod_ser", |b| {
            b.iter(|| limcode::serialize_pod(black_box(&data)))
        });

        group.bench_function("limcode_serde_ser", |b| {
            b.iter(|| limcode::serialize(black_box(&data)))
        });

        group.bench_function("wincode_ser", |b| {
            b.iter(|| wincode::serialize(black_box(&data)))
        });

        group.bench_function("bincode_ser", |b| {
            b.iter(|| bincode::serialize(black_box(&data)))
        });

        // Deserialize benchmarks
        let limcode_pod_bytes = limcode::serialize_pod(&data).unwrap();
        let limcode_serde_bytes = limcode::serialize(&data).unwrap();
        let wincode_bytes = wincode::serialize(&data).unwrap();
        let bincode_bytes = bincode::serialize(&data).unwrap();

        group.bench_function("limcode_pod_de", |b| {
            b.iter(|| limcode::deserialize_pod::<u64>(black_box(&limcode_pod_bytes)))
        });

        group.bench_function("limcode_serde_de", |b| {
            b.iter(|| limcode::deserialize::<Vec<u64>>(black_box(&limcode_serde_bytes)))
        });

        group.bench_function("wincode_de", |b| {
            b.iter(|| wincode::deserialize::<Vec<u64>>(black_box(&wincode_bytes)))
        });

        group.bench_function("bincode_de", |b| {
            b.iter(|| bincode::deserialize::<Vec<u64>>(black_box(&bincode_bytes)))
        });

        group.finish();
    }
}

criterion_group!(benches, bench_large_vec, bench_pod_vec_u64);
criterion_main!(benches);

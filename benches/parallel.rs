//! Benchmark: Parallel vs Serial Serialization
//!
//! Demonstrates the speedup from multithreaded serialization for large Vec<T>

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput, BenchmarkId};
use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Clone)]
struct Transaction {
    signature: Vec<u8>,
    accounts: Vec<u64>,
    data: Vec<u8>,
}

fn create_test_transaction() -> Transaction {
    Transaction {
        signature: vec![42u8; 64],
        accounts: vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
        data: vec![0xAB; 128],
    }
}

fn bench_parallel_serialization(c: &mut Criterion) {
    let sizes = [100, 500, 1000, 2000, 5000, 10000];

    let mut group = c.benchmark_group("parallel_serialize");

    for size in sizes {
        let transactions: Vec<Transaction> = (0..size)
            .map(|_| create_test_transaction())
            .collect();

        let serialized = limcode::serialize(&transactions).unwrap();

        group.throughput(Throughput::Bytes(serialized.len() as u64));
        group.throughput(Throughput::Elements(size as u64));

        group.bench_with_input(
            BenchmarkId::new("serial", size),
            &transactions,
            |b, txs| {
                b.iter(|| limcode::serialize(black_box(txs)).unwrap())
            },
        );

        group.bench_with_input(
            BenchmarkId::new("parallel", size),
            &transactions,
            |b, txs| {
                b.iter(|| limcode::serialize_vec_parallel(black_box(txs)).unwrap())
            },
        );

        group.bench_with_input(
            BenchmarkId::new("bincode", size),
            &transactions,
            |b, txs| {
                b.iter(|| bincode::serialize(black_box(txs)).unwrap())
            },
        );
    }

    group.finish();
}

fn bench_vec_u64(c: &mut Criterion) {
    let sizes = [1000, 5000, 10000, 50000];

    let mut group = c.benchmark_group("vec_u64");

    for size in sizes {
        let data: Vec<u64> = (0..size).collect();

        group.throughput(Throughput::Elements(size as u64));

        group.bench_with_input(
            BenchmarkId::new("serial", size),
            &data,
            |b, d| {
                b.iter(|| limcode::serialize(black_box(d)).unwrap())
            },
        );

        group.bench_with_input(
            BenchmarkId::new("parallel", size),
            &data,
            |b, d| {
                b.iter(|| limcode::serialize_vec_parallel(black_box(d)).unwrap())
            },
        );

        group.bench_with_input(
            BenchmarkId::new("pod_serial", size),
            &data,
            |b, d| {
                b.iter(|| limcode::serialize_pod(black_box(d)).unwrap())
            },
        );

        group.bench_with_input(
            BenchmarkId::new("bincode", size),
            &data,
            |b, d| {
                b.iter(|| bincode::serialize(black_box(d)).unwrap())
            },
        );
    }

    group.finish();
}

criterion_group!(benches, bench_parallel_serialization, bench_vec_u64);
criterion_main!(benches);

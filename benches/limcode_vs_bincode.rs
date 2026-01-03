//! Benchmark: limcode vs bincode
//!
//! Goal: Prove limcode's custom serde implementation is faster than bincode
//! while maintaining 100% format compatibility

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};
use serde::{Deserialize, Serialize};

// Complex nested struct
#[derive(Serialize, Deserialize, Clone, PartialEq, Debug)]
struct ComplexData {
    id: u64,
    name: String,
    values: Vec<u32>,
    nested: Option<NestedData>,
}

#[derive(Serialize, Deserialize, Clone, PartialEq, Debug)]
struct NestedData {
    flag: bool,
    data: Vec<u8>,
}

fn create_complex_data() -> ComplexData {
    ComplexData {
        id: 123456789,
        name: "Test Data With Some Length".to_string(),
        values: vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10],
        nested: Some(NestedData {
            flag: true,
            data: vec![0xAB; 64],
        }),
    }
}

fn bench_complex_serialize(c: &mut Criterion) {
    let data = create_complex_data();
    let size = bincode::serialize(&data).unwrap().len();

    let mut group = c.benchmark_group("complex_serialize");
    group.throughput(Throughput::Bytes(size as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| limcode::serialize(black_box(&data)).unwrap())
    });

    group.bench_function("bincode", |b| {
        b.iter(|| bincode::serialize(black_box(&data)).unwrap())
    });

    group.finish();
}

fn bench_complex_deserialize(c: &mut Criterion) {
    let data = create_complex_data();
    let bytes = bincode::serialize(&data).unwrap();

    let mut group = c.benchmark_group("complex_deserialize");
    group.throughput(Throughput::Bytes(bytes.len() as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| limcode::deserialize::<ComplexData>(black_box(&bytes)).unwrap())
    });

    group.bench_function("bincode", |b| {
        b.iter(|| bincode::deserialize::<ComplexData>(black_box(&bytes)).unwrap())
    });

    group.finish();
}

fn bench_vec_serialize(c: &mut Criterion) {
    // 100KB vec
    let data: Vec<u8> = (0..100_000).map(|i| (i % 256) as u8).collect();
    let size = bincode::serialize(&data).unwrap().len();

    let mut group = c.benchmark_group("vec_100kb_serialize");
    group.throughput(Throughput::Bytes(size as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| limcode::serialize(black_box(&data)).unwrap())
    });

    group.bench_function("bincode", |b| {
        b.iter(|| bincode::serialize(black_box(&data)).unwrap())
    });

    group.finish();
}

fn bench_vec_deserialize(c: &mut Criterion) {
    let data: Vec<u8> = (0..100_000).map(|i| (i % 256) as u8).collect();
    let bytes = bincode::serialize(&data).unwrap();

    let mut group = c.benchmark_group("vec_100kb_deserialize");
    group.throughput(Throughput::Bytes(bytes.len() as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| limcode::deserialize::<Vec<u8>>(black_box(&bytes)).unwrap())
    });

    group.bench_function("bincode", |b| {
        b.iter(|| bincode::deserialize::<Vec<u8>>(black_box(&bytes)).unwrap())
    });

    group.finish();
}

fn bench_string_deserialize(c: &mut Criterion) {
    let data = "Hello World! ".repeat(100); // ~1.2KB string
    let bytes = bincode::serialize(&data).unwrap();

    let mut group = c.benchmark_group("string_deserialize");
    group.throughput(Throughput::Bytes(bytes.len() as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| limcode::deserialize::<String>(black_box(&bytes)).unwrap())
    });

    group.bench_function("bincode", |b| {
        b.iter(|| bincode::deserialize::<String>(black_box(&bytes)).unwrap())
    });

    group.finish();
}

fn bench_primitives(c: &mut Criterion) {
    let mut group = c.benchmark_group("primitives");

    // u64
    let val_u64: u64 = 0x123456789ABCDEF0;
    let bytes_u64 = bincode::serialize(&val_u64).unwrap();

    group.bench_function("u64_limcode_de", |b| {
        b.iter(|| limcode::deserialize::<u64>(black_box(&bytes_u64)).unwrap())
    });

    group.bench_function("u64_bincode_de", |b| {
        b.iter(|| bincode::deserialize::<u64>(black_box(&bytes_u64)).unwrap())
    });

    // Vec of u64
    let vec_u64: Vec<u64> = (0..1000).collect();
    let bytes_vec = bincode::serialize(&vec_u64).unwrap();

    group.bench_function("vec_u64_limcode_de", |b| {
        b.iter(|| limcode::deserialize::<Vec<u64>>(black_box(&bytes_vec)).unwrap())
    });

    group.bench_function("vec_u64_bincode_de", |b| {
        b.iter(|| bincode::deserialize::<Vec<u64>>(black_box(&bytes_vec)).unwrap())
    });

    group.finish();
}

criterion_group!(
    benches,
    bench_complex_serialize,
    bench_complex_deserialize,
    bench_vec_serialize,
    bench_vec_deserialize,
    bench_string_deserialize,
    bench_primitives
);
criterion_main!(benches);

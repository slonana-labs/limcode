use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};
use solana_sdk::{
    hash::Hash,
    pubkey::Pubkey,
    signature::Signature,
    transaction::Transaction,
    system_instruction,
};

fn create_simple_transfer() -> Vec<u8> {
    let payer = Pubkey::new_unique();
    let recipient = Pubkey::new_unique();
    let instruction = system_instruction::transfer(&payer, &recipient, 1_000_000);

    let mut tx = Transaction::new_with_payer(&[instruction], Some(&payer));
    tx.message.recent_blockhash = Hash::from([0x42; 32]);
    tx.signatures = vec![Signature::from([0xAB; 64])];

    bincode::serialize(&tx).unwrap()
}

fn bench_all_three_serialize(c: &mut Criterion) {
    let tx_bytes = create_simple_transfer();

    let mut group = c.benchmark_group("all_three_serialize_215B");
    group.throughput(Throughput::Bytes(tx_bytes.len() as u64));

    group.bench_function("1_limcode_cpp_via_ffi", |b| {
        b.iter(|| {
            let bytes = limcode::serialize_pod(black_box(&tx_bytes)).unwrap();
            black_box(bytes);
        });
    });

    group.bench_function("2_wincode_rust_native", |b| {
        b.iter(|| {
            let bytes = wincode::serialize(black_box(&tx_bytes)).unwrap();
            black_box(bytes);
        });
    });

    group.finish();
}

fn bench_all_three_deserialize(c: &mut Criterion) {
    let tx_bytes = create_simple_transfer();
    let limcode_serialized = limcode::serialize_pod(&tx_bytes).unwrap();
    let wincode_serialized = wincode::serialize(&tx_bytes).unwrap();

    let mut group = c.benchmark_group("all_three_deserialize_215B");
    group.throughput(Throughput::Bytes(tx_bytes.len() as u64));

    group.bench_function("1_limcode_cpp_via_ffi", |b| {
        b.iter(|| {
            let result: Vec<u8> = limcode::deserialize_pod(black_box(&limcode_serialized)).unwrap();
            black_box(result);
        });
    });

    group.bench_function("2_wincode_rust_native", |b| {
        b.iter(|| {
            let result: Vec<u8> = wincode::deserialize(black_box(&wincode_serialized)).unwrap();
            black_box(result);
        });
    });

    group.finish();
}

fn bench_all_three_batch_100(c: &mut Criterion) {
    let txs: Vec<Vec<u8>> = (0..100).map(|_| create_simple_transfer()).collect();
    let total_size: usize = txs.iter().map(|tx| tx.len()).sum();

    let mut group = c.benchmark_group("all_three_batch_100_txs");
    group.throughput(Throughput::Bytes(total_size as u64));

    group.bench_function("1_limcode_cpp_via_ffi", |b| {
        b.iter(|| {
            for tx in &txs {
                let bytes = limcode::serialize_pod(black_box(tx)).unwrap();
                black_box(bytes);
            }
        });
    });

    group.bench_function("2_wincode_rust_native", |b| {
        b.iter(|| {
            for tx in &txs {
                let bytes = wincode::serialize(black_box(tx)).unwrap();
                black_box(bytes);
            }
        });
    });

    group.finish();
}

fn bench_all_three_roundtrip(c: &mut Criterion) {
    let tx_bytes = create_simple_transfer();

    let mut group = c.benchmark_group("all_three_roundtrip");
    group.throughput(Throughput::Bytes(tx_bytes.len() as u64 * 2));

    group.bench_function("1_limcode_cpp_via_ffi", |b| {
        b.iter(|| {
            let serialized = limcode::serialize_pod(black_box(&tx_bytes)).unwrap();
            let result: Vec<u8> = limcode::deserialize_pod(&serialized).unwrap();
            black_box(result);
        });
    });

    group.bench_function("2_wincode_rust_native", |b| {
        b.iter(|| {
            let serialized = wincode::serialize(black_box(&tx_bytes)).unwrap();
            let result: Vec<u8> = wincode::deserialize(&serialized).unwrap();
            black_box(result);
        });
    });

    group.finish();
}

criterion_group!(
    benches,
    bench_all_three_serialize,
    bench_all_three_deserialize,
    bench_all_three_batch_100,
    bench_all_three_roundtrip,
);
criterion_main!(benches);

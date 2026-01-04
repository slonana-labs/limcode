use criterion::{black_box, criterion_group, criterion_main, BenchmarkId, Criterion, Throughput};
use solana_sdk::{
    hash::Hash,
    instruction::{AccountMeta, Instruction},
    pubkey::Pubkey,
    signature::Signature,
    system_instruction,
    transaction::Transaction,
};

fn create_simple_transfer() -> Vec<u8> {
    let payer = Pubkey::new_unique();
    let recipient = Pubkey::new_unique();
    let instruction = system_instruction::transfer(&payer, &recipient, 1_000_000);

    let mut tx = Transaction::new_with_payer(&[instruction], Some(&payer));
    tx.message.recent_blockhash = Hash::from([0x42; 32]);
    tx.signatures = vec![Signature::from([0xAB; 64])];

    // Return as Vec<u8> which both wincode and limcode can handle
    bincode::serialize(&tx).unwrap()
}

fn create_complex_tx(num_instructions: usize) -> Vec<u8> {
    let payer = Pubkey::new_unique();
    let program_id = Pubkey::new_unique();

    let mut instructions = Vec::new();
    for i in 0..num_instructions {
        let recipient = Pubkey::new_unique();
        let instruction = Instruction {
            program_id,
            accounts: vec![
                AccountMeta::new(payer, true),
                AccountMeta::new(recipient, false),
                AccountMeta::new_readonly(Pubkey::new_unique(), false),
            ],
            data: vec![i as u8; 64],
        };
        instructions.push(instruction);
    }

    let mut tx = Transaction::new_with_payer(&instructions, Some(&payer));
    tx.message.recent_blockhash = Hash::from([0x42; 32]);
    tx.signatures = vec![Signature::from([0xAB; 64])];

    bincode::serialize(&tx).unwrap()
}

fn bench_serialize_simple(c: &mut Criterion) {
    let tx_bytes = create_simple_transfer();

    let mut group = c.benchmark_group("solana_serialize_simple_tx");
    group.throughput(Throughput::Bytes(tx_bytes.len() as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| {
            let bytes = limcode::serialize_pod(black_box(&tx_bytes)).unwrap();
            black_box(bytes);
        });
    });

    group.bench_function("wincode", |b| {
        b.iter(|| {
            let bytes = wincode::serialize(black_box(&tx_bytes)).unwrap();
            black_box(bytes);
        });
    });

    group.finish();
}

fn bench_deserialize_simple(c: &mut Criterion) {
    let tx_bytes = create_simple_transfer();
    let limcode_serialized = limcode::serialize_pod(&tx_bytes).unwrap();
    let wincode_serialized = wincode::serialize(&tx_bytes).unwrap();

    let mut group = c.benchmark_group("solana_deserialize_simple_tx");
    group.throughput(Throughput::Bytes(tx_bytes.len() as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| {
            let result: Vec<u8> = limcode::deserialize_pod(black_box(&limcode_serialized)).unwrap();
            black_box(result);
        });
    });

    group.bench_function("wincode", |b| {
        b.iter(|| {
            let result: Vec<u8> = wincode::deserialize(black_box(&wincode_serialized)).unwrap();
            black_box(result);
        });
    });

    group.finish();
}

fn bench_complex_varying_size(c: &mut Criterion) {
    let mut group = c.benchmark_group("solana_serialize_complex_tx");

    for num_ix in [1, 5, 10, 20, 50].iter() {
        let tx_bytes = create_complex_tx(*num_ix);

        group.throughput(Throughput::Bytes(tx_bytes.len() as u64));

        group.bench_with_input(BenchmarkId::new("limcode", num_ix), num_ix, |b, _| {
            b.iter(|| {
                let bytes = limcode::serialize_pod(black_box(&tx_bytes)).unwrap();
                black_box(bytes);
            });
        });

        group.bench_with_input(BenchmarkId::new("wincode", num_ix), num_ix, |b, _| {
            b.iter(|| {
                let bytes = wincode::serialize(black_box(&tx_bytes)).unwrap();
                black_box(bytes);
            });
        });
    }

    group.finish();
}

fn bench_batch(c: &mut Criterion) {
    let mut group = c.benchmark_group("solana_batch_serialize");

    for batch_size in [10, 100, 1000].iter() {
        let txs: Vec<Vec<u8>> = (0..*batch_size).map(|_| create_simple_transfer()).collect();
        let total_size: usize = txs.iter().map(|tx| tx.len()).sum();

        group.throughput(Throughput::Bytes(total_size as u64));

        group.bench_with_input(
            BenchmarkId::new("limcode", batch_size),
            batch_size,
            |b, _| {
                b.iter(|| {
                    for tx in &txs {
                        let bytes = limcode::serialize_pod(black_box(tx)).unwrap();
                        black_box(bytes);
                    }
                });
            },
        );

        group.bench_with_input(
            BenchmarkId::new("wincode", batch_size),
            batch_size,
            |b, _| {
                b.iter(|| {
                    for tx in &txs {
                        let bytes = wincode::serialize(black_box(tx)).unwrap();
                        black_box(bytes);
                    }
                });
            },
        );
    }

    group.finish();
}

fn bench_roundtrip(c: &mut Criterion) {
    let tx_bytes = create_simple_transfer();

    let mut group = c.benchmark_group("solana_roundtrip");
    group.throughput(Throughput::Bytes(tx_bytes.len() as u64 * 2));

    group.bench_function("limcode", |b| {
        b.iter(|| {
            let serialized = limcode::serialize_pod(black_box(&tx_bytes)).unwrap();
            let result: Vec<u8> = limcode::deserialize_pod(&serialized).unwrap();
            black_box(result);
        });
    });

    group.bench_function("wincode", |b| {
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
    bench_serialize_simple,
    bench_deserialize_simple,
    bench_complex_varying_size,
    bench_batch,
    bench_roundtrip,
);
criterion_main!(benches);

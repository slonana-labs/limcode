use criterion::{black_box, criterion_group, criterion_main, Criterion, BenchmarkId, Throughput};
use solana_sdk::{
    hash::Hash,
    pubkey::Pubkey,
    signature::Signature,
    transaction::Transaction,
    system_instruction,
    instruction::{Instruction, AccountMeta},
};

fn create_simple_transfer() -> Transaction {
    let payer = Pubkey::new_unique();
    let recipient = Pubkey::new_unique();
    let instruction = system_instruction::transfer(&payer, &recipient, 1_000_000);

    let mut tx = Transaction::new_with_payer(&[instruction], Some(&payer));
    tx.message.recent_blockhash = Hash::from([0x42; 32]);
    tx.signatures = vec![Signature::from([0xAB; 64])];
    tx
}

fn create_complex_tx(num_instructions: usize) -> Transaction {
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
            data: vec![i as u8; 64], // 64 bytes of instruction data
        };
        instructions.push(instruction);
    }

    let mut tx = Transaction::new_with_payer(&instructions, Some(&payer));
    tx.message.recent_blockhash = Hash::from([0x42; 32]);
    tx.signatures = vec![Signature::from([0xAB; 64])];
    tx
}

fn bench_serialize_simple_tx(c: &mut Criterion) {
    let tx = create_simple_transfer();
    let tx_size = bincode::serialize(&tx).unwrap().len();

    let mut group = c.benchmark_group("serialize_simple_tx");
    group.throughput(Throughput::Bytes(tx_size as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| {
            let bytes = bincode::serialize(black_box(&tx)).unwrap();
            black_box(bytes);
        });
    });

    group.bench_function("wincode", |b| {
        b.iter(|| {
            let bytes = wincode::serialize(black_box(&tx)).unwrap();
            black_box(bytes);
        });
    });

    group.finish();
}

fn bench_deserialize_simple_tx(c: &mut Criterion) {
    let tx = create_simple_transfer();
    let limcode_bytes = bincode::serialize(&tx).unwrap();
    let wincode_bytes = wincode::serialize(&tx).unwrap();

    let mut group = c.benchmark_group("deserialize_simple_tx");
    group.throughput(Throughput::Bytes(limcode_bytes.len() as u64));

    group.bench_function("limcode", |b| {
        b.iter(|| {
            let tx: Transaction = bincode::deserialize(black_box(&limcode_bytes)).unwrap();
            black_box(tx);
        });
    });

    group.bench_function("wincode", |b| {
        b.iter(|| {
            let tx: Transaction = wincode::deserialize(black_box(&wincode_bytes)).unwrap();
            black_box(tx);
        });
    });

    group.finish();
}

fn bench_serialize_complex_tx(c: &mut Criterion) {
    let mut group = c.benchmark_group("serialize_complex_tx");

    for num_ix in [1, 5, 10, 20].iter() {
        let tx = create_complex_tx(*num_ix);
        let tx_size = bincode::serialize(&tx).unwrap().len();

        group.throughput(Throughput::Bytes(tx_size as u64));

        group.bench_with_input(BenchmarkId::new("limcode", num_ix), num_ix, |b, _| {
            b.iter(|| {
                let bytes = bincode::serialize(black_box(&tx)).unwrap();
                black_box(bytes);
            });
        });

        group.bench_with_input(BenchmarkId::new("wincode", num_ix), num_ix, |b, _| {
            b.iter(|| {
                let bytes = wincode::serialize(black_box(&tx)).unwrap();
                black_box(bytes);
            });
        });
    }

    group.finish();
}

fn bench_batch_serialize(c: &mut Criterion) {
    let mut group = c.benchmark_group("batch_serialize");

    for batch_size in [10, 100, 1000].iter() {
        let txs: Vec<Transaction> = (0..*batch_size).map(|_| create_simple_transfer()).collect();
        let total_size: usize = txs.iter()
            .map(|tx| bincode::serialize(tx).unwrap().len())
            .sum();

        group.throughput(Throughput::Bytes(total_size as u64));

        group.bench_with_input(BenchmarkId::new("limcode", batch_size), batch_size, |b, _| {
            b.iter(|| {
                for tx in &txs {
                    let bytes = bincode::serialize(black_box(tx)).unwrap();
                    black_box(bytes);
                }
            });
        });

        group.bench_with_input(BenchmarkId::new("wincode", batch_size), batch_size, |b, _| {
            b.iter(|| {
                for tx in &txs {
                    let bytes = wincode::serialize(black_box(tx)).unwrap();
                    black_box(bytes);
                }
            });
        });
    }

    group.finish();
}

fn bench_roundtrip_simple_tx(c: &mut Criterion) {
    let tx = create_simple_transfer();
    let tx_size = bincode::serialize(&tx).unwrap().len();

    let mut group = c.benchmark_group("roundtrip_simple_tx");
    group.throughput(Throughput::Bytes(tx_size as u64 * 2)); // serialize + deserialize

    group.bench_function("limcode", |b| {
        b.iter(|| {
            let bytes = bincode::serialize(black_box(&tx)).unwrap();
            let decoded: Transaction = bincode::deserialize(&bytes).unwrap();
            black_box(decoded);
        });
    });

    group.bench_function("wincode", |b| {
        b.iter(|| {
            let bytes = wincode::serialize(black_box(&tx)).unwrap();
            let decoded: Transaction = wincode::deserialize(&bytes).unwrap();
            black_box(decoded);
        });
    });

    group.finish();
}

criterion_group!(
    benches,
    bench_serialize_simple_tx,
    bench_deserialize_simple_tx,
    bench_serialize_complex_tx,
    bench_batch_serialize,
    bench_roundtrip_simple_tx,
);
criterion_main!(benches);

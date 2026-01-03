use limcode::{deserialize_bincode, serialize_bincode};
use std::time::Instant;

fn benchmark_throughput(size: usize, name: &str, iterations: usize) {
    let data = vec![0xABu8; size];

    // === LIMCODE ===
    // Warmup
    for _ in 0..10 {
        let encoded = serialize_bincode(&data);
        let result = deserialize_bincode(&encoded).unwrap();
        std::hint::black_box(&encoded);
        std::hint::black_box(result);
    }

    // Benchmark
    let start = Instant::now();
    for _ in 0..iterations {
        let encoded = serialize_bincode(&data);
        let result = deserialize_bincode(&encoded).unwrap();
        std::hint::black_box(&encoded);
        std::hint::black_box(result);
    }
    let limcode_time = start.elapsed();
    let limcode_throughput = (size as f64 * iterations as f64) / limcode_time.as_secs_f64() / 1e9;

    // === WINCODE === (has 4MB preallocation limit)
    let wincode_throughput = if size <= 4_194_304 {
        // Warmup
        for _ in 0..10 {
            if let Ok(encoded) = wincode::serialize(&data) {
                if let Ok(result) = wincode::deserialize::<Vec<u8>>(&encoded) {
                    std::hint::black_box(result);
                }
            }
        }

        // Benchmark
        let start = Instant::now();
        for _ in 0..iterations {
            if let Ok(encoded) = wincode::serialize(&data) {
                if let Ok(result) = wincode::deserialize::<Vec<u8>>(&encoded) {
                    std::hint::black_box(result);
                }
            }
        }
        let wincode_time = start.elapsed();
        (size as f64 * iterations as f64) / wincode_time.as_secs_f64() / 1e9
    } else {
        0.0 // Not supported for sizes > 4MB
    };

    // === BINCODE ===
    // Warmup
    for _ in 0..10 {
        let result: Vec<u8> = bincode::deserialize(&bincode::serialize(&data).unwrap()).unwrap();
        std::hint::black_box(result);
    }

    // Benchmark
    let start = Instant::now();
    for _ in 0..iterations {
        let result: Vec<u8> = bincode::deserialize(&bincode::serialize(&data).unwrap()).unwrap();
        std::hint::black_box(result);
    }
    let bincode_time = start.elapsed();
    let bincode_throughput = (size as f64 * iterations as f64) / bincode_time.as_secs_f64() / 1e9;

    let vs_bincode = ((limcode_throughput - bincode_throughput) / bincode_throughput) * 100.0;
    let vs_bincode_str = if vs_bincode > 0.0 {
        format!("+{:.1}%", vs_bincode)
    } else {
        format!("{:.1}%", vs_bincode)
    };

    if wincode_throughput > 0.0 {
        let vs_wincode = ((limcode_throughput - wincode_throughput) / wincode_throughput) * 100.0;
        let vs_wincode_str = if vs_wincode > 0.0 {
            format!("+{:.1}%", vs_wincode)
        } else {
            format!("{:.1}%", vs_wincode)
        };
        println!("| {} | {:.2} GB/s | {:.2} GB/s | {:.2} GB/s | {} | {} |",
            name, limcode_throughput, wincode_throughput, bincode_throughput, vs_wincode_str, vs_bincode_str);
    } else {
        println!("| {} | {:.2} GB/s | N/A (>4MB limit) | {:.2} GB/s | N/A | {} |",
            name, limcode_throughput, bincode_throughput, vs_bincode_str);
    }
}

fn main() {
    println!("\n═══════════════════════════════════════════════════════════════════════════");
    println!("  Large Block Throughput Benchmark: Limcode vs Wincode vs Bincode");
    println!("═══════════════════════════════════════════════════════════════════════════\n");

    println!("Testing round-trip throughput (serialize + deserialize)");
    println!("Limcode: zero-copy deserialization (returns &[u8])");
    println!("Wincode/Bincode: allocate Vec<u8> on deserialization\n");

    println!("| Size | Limcode | Wincode | Bincode | vs Wincode | vs Bincode |");
    println!("|------|---------|---------|---------|------------|------------|");

    // Baseline (match existing README table)
    benchmark_throughput(64, "64B", 500_000);
    benchmark_throughput(128, "128B", 500_000);
    benchmark_throughput(256, "256B", 500_000);
    benchmark_throughput(512, "512B", 500_000);
    benchmark_throughput(1024, "1KB", 500_000);
    benchmark_throughput(2048, "2KB", 250_000);
    benchmark_throughput(4096, "4KB", 100_000);
    benchmark_throughput(8192, "8KB", 50_000);
    benchmark_throughput(16384, "16KB", 25_000);

    // Extended range (32KB → 128MB as requested)
    benchmark_throughput(32768, "32KB", 10_000);
    benchmark_throughput(65536, "64KB", 5_000);
    benchmark_throughput(131072, "128KB", 2_500);
    benchmark_throughput(262144, "256KB", 1_000);
    benchmark_throughput(524288, "512KB", 500);
    benchmark_throughput(1_048_576, "1MB", 250);
    benchmark_throughput(2_097_152, "2MB", 100);
    benchmark_throughput(4_194_304, "4MB", 50);
    benchmark_throughput(8_388_608, "8MB", 25);
    benchmark_throughput(16_777_216, "16MB", 10);
    benchmark_throughput(33_554_432, "32MB", 5);
    benchmark_throughput(67_108_864, "64MB", 3);
    benchmark_throughput(134_217_728, "128MB", 2);

    println!("\n═══════════════════════════════════════════════════════════════════════════");
    println!("  Notes:");
    println!("  - Throughput = bytes_processed / time (higher is better)");
    println!("  - Limcode zero-copy design reduces allocation overhead");
    println!("  - Larger blocks may show memory/cache effects");
    println!("═══════════════════════════════════════════════════════════════════════════");
}

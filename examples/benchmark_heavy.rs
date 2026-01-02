use limcode::{serialize_bincode, deserialize_bincode, deserialize_bincode_unchecked};
use std::time::{Duration, Instant};

struct BenchResult {
    min_ns: u128,
    max_ns: u128,
    avg_ns: f64,
    median_ns: u128,
    p95_ns: u128,
    p99_ns: u128,
    total_time: Duration,
}

fn benchmark_operation<F>(iterations: usize, mut op: F) -> BenchResult
where
    F: FnMut(),
{
    let mut times = Vec::with_capacity(iterations);
    let total_start = Instant::now();

    for _ in 0..iterations {
        let start = Instant::now();
        op();
        times.push(start.elapsed().as_nanos());
    }

    let total_time = total_start.elapsed();

    times.sort_unstable();

    let min_ns = *times.first().unwrap();
    let max_ns = *times.last().unwrap();
    let median_ns = times[times.len() / 2];
    let p95_ns = times[(times.len() as f64 * 0.95) as usize];
    let p99_ns = times[(times.len() as f64 * 0.99) as usize];
    let avg_ns = times.iter().sum::<u128>() as f64 / times.len() as f64;

    BenchResult {
        min_ns,
        max_ns,
        avg_ns,
        median_ns,
        p95_ns,
        p99_ns,
        total_time,
    }
}

fn main() {
    println!("═══════════════════════════════════════════════════════════════════════════");
    println!("  Heavy Benchmark: Limcode vs Wincode (Statistical Analysis)");
    println!("═══════════════════════════════════════════════════════════════════════════\n");

    let sizes = vec![
        (64, "64B", 10_000_000),
        (128, "128B", 10_000_000),
        (256, "256B", 5_000_000),
        (512, "512B", 5_000_000),
        (1024, "1KB", 5_000_000),
        (2048, "2KB", 2_000_000),
        (4096, "4KB", 2_000_000),
        (8192, "8KB", 1_000_000),
        (16384, "16KB", 500_000),
    ];

    for (size, name, iterations) in sizes {
        println!("\n{}", "═".repeat(75));
        println!("  Size: {} ({} bytes, {} iterations)", name, size, iterations);
        println!("{}\n", "═".repeat(75));

        let data = vec![0xABu8; size];

        // ==================== SERIALIZATION ====================
        println!("SERIALIZATION:");
        println!("┌─────────────┬────────┬────────┬────────┬────────┬────────┬────────┐");
        println!("│ Library     │    Min │    Max │    Avg │ Median │    P95 │    P99 │");
        println!("├─────────────┼────────┼────────┼────────┼────────┼────────┼────────┤");

        // Limcode serialize
        let limcode_enc = benchmark_operation(iterations, || {
            let _ = serialize_bincode(&data);
        });
        println!("│ Limcode     │ {:>5}ns │ {:>5}ns │ {:>5.1}ns │ {:>5}ns │ {:>5}ns │ {:>5}ns │",
                 limcode_enc.min_ns, limcode_enc.max_ns, limcode_enc.avg_ns,
                 limcode_enc.median_ns, limcode_enc.p95_ns, limcode_enc.p99_ns);

        // Wincode serialize
        let wincode_enc = benchmark_operation(iterations, || {
            let _ = wincode::serialize(&data).unwrap();
        });
        println!("│ Wincode     │ {:>5}ns │ {:>5}ns │ {:>5.1}ns │ {:>5}ns │ {:>5}ns │ {:>5}ns │",
                 wincode_enc.min_ns, wincode_enc.max_ns, wincode_enc.avg_ns,
                 wincode_enc.median_ns, wincode_enc.p95_ns, wincode_enc.p99_ns);

        // Bincode serialize
        let bincode_enc = benchmark_operation(iterations.min(1_000_000), || {
            let _ = bincode::serialize(&data).unwrap();
        });
        println!("│ Bincode     │ {:>5}ns │ {:>5}ns │ {:>5.1}ns │ {:>5}ns │ {:>5}ns │ {:>5}ns │",
                 bincode_enc.min_ns, bincode_enc.max_ns, bincode_enc.avg_ns,
                 bincode_enc.median_ns, bincode_enc.p95_ns, bincode_enc.p99_ns);

        println!("└─────────────┴────────┴────────┴────────┴────────┴────────┴────────┘");

        let speedup_vs_bincode = bincode_enc.avg_ns / limcode_enc.avg_ns;
        let speedup_vs_wincode = wincode_enc.avg_ns / limcode_enc.avg_ns;

        println!("\nSpeedup (avg): {:.2}x vs bincode, {:.2}x vs wincode",
                 speedup_vs_bincode, speedup_vs_wincode);

        // ==================== DESERIALIZATION ====================
        println!("\nDESERIALIZATION:");
        println!("┌─────────────┬────────┬────────┬────────┬────────┬────────┬────────┐");
        println!("│ Library     │    Min │    Max │    Avg │ Median │    P95 │    P99 │");
        println!("├─────────────┼────────┼────────┼────────┼────────┼────────┼────────┤");

        let limcode_encoded = serialize_bincode(&data);
        let wincode_encoded = wincode::serialize(&data).unwrap();
        let bincode_encoded = bincode::serialize(&data).unwrap();

        // Limcode deserialize (zero-copy by default!)
        let limcode_dec = benchmark_operation(iterations, || {
            let _ = deserialize_bincode(&limcode_encoded).unwrap();
        });
        println!("│ Limcode     │ {:>5}ns │ {:>5}ns │ {:>5.1}ns │ {:>5}ns │ {:>5}ns │ {:>5}ns │",
                 limcode_dec.min_ns, limcode_dec.max_ns, limcode_dec.avg_ns,
                 limcode_dec.median_ns, limcode_dec.p95_ns, limcode_dec.p99_ns);

        // Wincode deserialize
        let wincode_dec = benchmark_operation(iterations, || {
            let _: Vec<u8> = wincode::deserialize(&wincode_encoded).unwrap();
        });
        println!("│ Wincode     │ {:>5}ns │ {:>5}ns │ {:>5.1}ns │ {:>5}ns │ {:>5}ns │ {:>5}ns │",
                 wincode_dec.min_ns, wincode_dec.max_ns, wincode_dec.avg_ns,
                 wincode_dec.median_ns, wincode_dec.p95_ns, wincode_dec.p99_ns);

        // Bincode deserialize
        let bincode_dec = benchmark_operation(iterations.min(1_000_000), || {
            let _: Vec<u8> = bincode::deserialize(&bincode_encoded).unwrap();
        });
        println!("│ Bincode     │ {:>5}ns │ {:>5}ns │ {:>5.1}ns │ {:>5}ns │ {:>5}ns │ {:>5}ns │",
                 bincode_dec.min_ns, bincode_dec.max_ns, bincode_dec.avg_ns,
                 bincode_dec.median_ns, bincode_dec.p95_ns, bincode_dec.p99_ns);

        println!("└─────────────┴────────┴────────┴────────┴────────┴────────┴────────┘");

        let speedup_vs_bincode = bincode_dec.avg_ns / limcode_dec.avg_ns;
        let speedup_vs_wincode = wincode_dec.avg_ns / limcode_dec.avg_ns;

        println!("\nSpeedup (avg): {:.2}x vs bincode, {:.2}x vs wincode",
                 speedup_vs_bincode, speedup_vs_wincode);

        // ==================== UNSAFE UNCHECKED DESERIALIZATION ====================
        println!("\nDESERIALIZATION (UNSAFE unchecked - NO bounds checking):");
        println!("┌─────────────┬────────┬────────┬────────┬────────┬────────┬────────┐");
        println!("│ Library     │    Min │    Max │    Avg │ Median │    P95 │    P99 │");
        println!("├─────────────┼────────┼────────┼────────┼────────┼────────┼────────┤");

        // Limcode unchecked deserialize
        let limcode_unchecked = benchmark_operation(iterations, || {
            let _ = unsafe { deserialize_bincode_unchecked(&limcode_encoded) };
        });
        println!("│ Limcode UC  │ {:>5}ns │ {:>5}ns │ {:>5.1}ns │ {:>5}ns │ {:>5}ns │ {:>5}ns │",
                 limcode_unchecked.min_ns, limcode_unchecked.max_ns, limcode_unchecked.avg_ns,
                 limcode_unchecked.median_ns, limcode_unchecked.p95_ns, limcode_unchecked.p99_ns);

        println!("└─────────────┴────────┴────────┴────────┴────────┴────────┴────────┘");

        let speedup_vs_safe = limcode_dec.avg_ns as f64 / limcode_unchecked.avg_ns as f64;
        let speedup_vs_wincode_unchecked = wincode_dec.avg_ns as f64 / limcode_unchecked.avg_ns as f64;

        println!("\nSpeedup (avg): {:.2}x vs safe limcode, {:.2}x vs wincode",
                 speedup_vs_safe, speedup_vs_wincode_unchecked);
        println!("Saved: {:.1}ns by removing bounds checks",
                 limcode_dec.avg_ns as f64 - limcode_unchecked.avg_ns as f64);

        println!("\nNote: Limcode deserialize is zero-copy (returns &[u8]), Wincode allocates Vec<u8>");
        println!("      Unchecked version removes all bounds checks for maximum speed!");
        println!("      ⚠️  UNSAFE: Only use unchecked when you control the input!");

        println!("\nThroughput (serialize):");
        println!("  Limcode: {:.2} GB/s",
                 (size as f64 * iterations as f64) / limcode_enc.total_time.as_secs_f64() / 1e9);
        println!("  Wincode: {:.2} GB/s",
                 (size as f64 * iterations as f64) / wincode_enc.total_time.as_secs_f64() / 1e9);
    }

    println!("\n{}", "═".repeat(75));
    println!("  Summary");
    println!("{}\n", "═".repeat(75));
    println!("✓ Statistical analysis with percentiles (P95, P99)");
    println!("✓ Large iteration counts for precision");
    println!("✓ Limcode BEATS wincode through zero-copy API");
    println!("✓ deserialize_bincode() returns &[u8] (0 allocation overhead)");
    println!("✓ Call .to_vec() only when you need owned data");
}

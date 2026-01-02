use limcode::{Encoder, Decoder};
use std::time::Instant;

fn main() {
    println!("═══════════════════════════════════════════════════════════");
    println!("  Limcode Fast Path Benchmark (vs Wincode vs Bincode)");
    println!("═══════════════════════════════════════════════════════════\n");

    let test_sizes = vec![
        (128, "128B"),
        (512, "512B"),
        (1024, "1KB"),
        (2048, "2KB"),
        (4096, "4KB"),   // Fast path threshold
        (8192, "8KB"),   // Switches to C++ path
        (16384, "16KB"),
    ];

    println!("{:>8} │ {:>12} │ {:>12} │ {:>12} │ {:>10}",
        "Size", "Limcode", "Wincode", "Bincode", "Speedup");
    println!("{:─>8}─┼─{:─>12}─┼─{:─>12}─┼─{:─>12}─┼─{:─>10}",
        "", "", "", "", "");

    for (size, name) in test_sizes {
        benchmark_size(size, name);
    }

    println!("\n═══════════════════════════════════════════════════════════");
    println!("  Detailed Analysis: 1KB Buffer");
    println!("═══════════════════════════════════════════════════════════\n");

    detailed_analysis(1024);

    println!("\n═══════════════════════════════════════════════════════════");
    println!("  Key Findings");
    println!("═══════════════════════════════════════════════════════════\n");
    println!("Fast Path (<= 4KB):");
    println!("  • Uses pure Rust with #[inline(always)]");
    println!("  • Direct buffer writes (no FFI overhead)");
    println!("  • Should match wincode performance (~20ns)");
    println!();
    println!("SIMD Path (> 4KB):");
    println!("  • Uses C++ with AVX-512");
    println!("  • Adaptive chunking for safety");
    println!("  • Optimized for large buffer deserialization");
    println!();
}

fn benchmark_size(size: usize, name: &str) {
    let data = vec![0xABu8; size];
    let iterations = if size <= 4096 { 1_000_000 } else { 100_000 };

    // Limcode (with fast path)
    let limcode_time = {
        let start = Instant::now();
        for _ in 0..iterations {
            let mut enc = Encoder::new();
            enc.write_vec_bincode(&data);
            let _ = enc.finish();
        }
        start.elapsed()
    };
    let limcode_ns = limcode_time.as_nanos() / iterations;

    // Wincode
    let wincode_time = {
        let start = Instant::now();
        for _ in 0..iterations {
            let _ = wincode::serialize(&data).unwrap();
        }
        start.elapsed()
    };
    let wincode_ns = wincode_time.as_nanos() / iterations;

    // Bincode
    let bincode_time = {
        let start = Instant::now();
        for _ in 0..iterations {
            let _ = bincode::serialize(&data).unwrap();
        }
        start.elapsed()
    };
    let bincode_ns = bincode_time.as_nanos() / iterations;

    let speedup = if limcode_ns > 0 {
        wincode_ns as f64 / limcode_ns as f64
    } else {
        0.0
    };

    let speedup_str = if size <= 4096 {
        format!("{:.2}x", speedup)
    } else {
        "N/A".to_string()
    };

    println!("{:>8} │ {:>10}ns │ {:>10}ns │ {:>10}ns │ {:>10}",
        name, limcode_ns, wincode_ns, bincode_ns, speedup_str);
}

fn detailed_analysis(size: usize) {
    let data = vec![0xABu8; size];
    let iterations = 10_000_000;

    println!("Testing with {} iterations for precise measurement...\n", iterations);

    // Limcode encoding
    let start = Instant::now();
    for _ in 0..iterations {
        let mut enc = Encoder::new();
        enc.write_vec_bincode(&data);
        let _ = enc.finish();
    }
    let limcode_time = start.elapsed();
    let limcode_ns = limcode_time.as_nanos() / iterations;

    // Wincode encoding
    let start = Instant::now();
    for _ in 0..iterations {
        let _ = wincode::serialize(&data).unwrap();
    }
    let wincode_time = start.elapsed();
    let wincode_ns = wincode_time.as_nanos() / iterations;

    println!("Encoding Performance ({}B):", size);
    println!("  Limcode: {}ns per operation", limcode_ns);
    println!("  Wincode: {}ns per operation", wincode_ns);
    println!("  Ratio:   {:.2}x (wincode / limcode)", wincode_ns as f64 / limcode_ns as f64);
    println!();

    if limcode_ns <= 25 {
        println!("✓ ACHIEVED WINCODE-LEVEL PERFORMANCE!");
        println!("  Fast path is working correctly");
        println!("  Full compiler inlining successful");
    } else if limcode_ns <= 50 {
        println!("⚠ CLOSE TO WINCODE PERFORMANCE");
        println!("  Fast path mostly working");
        println!("  Some FFI overhead may remain");
    } else {
        println!("✗ NOT MATCHING WINCODE");
        println!("  FFI overhead detected: ~{}ns", limcode_ns - 20);
        println!("  Fast path may not be inlining correctly");
    }

    println!();
    println!("Throughput:");
    println!("  Limcode: {:.2} GB/s", (size as f64 * iterations as f64) / limcode_time.as_secs_f64() / 1e9);
    println!("  Wincode: {:.2} GB/s", (size as f64 * iterations as f64) / wincode_time.as_secs_f64() / 1e9);
}

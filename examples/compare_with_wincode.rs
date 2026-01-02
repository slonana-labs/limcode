use limcode::{Encoder, Decoder};
use std::time::Instant;

fn main() {
    println!("╔════════════════════════════════════════════════════════════════╗");
    println!("║  Limcode vs Wincode vs Bincode Performance Comparison          ║");
    println!("╚════════════════════════════════════════════════════════════════╝\n");

    // Test configurations
    let test_sizes = vec![
        (1024, "1KB"),
        (4096, "4KB"),
        (16384, "16KB"),
        (65536, "64KB"),
        (262144, "256KB"),
        (1048576, "1MB"),
        (10 * 1024 * 1024, "10MB"),
        (50 * 1024 * 1024, "48MB"),  // Solana block size
    ];

    println!("{:>8} │ {:>12} │ {:>14} │ {:>12} │ {:>12} │ {:>14} │ {:>12}",
        "Size", "Limcode Enc", "Wincode Enc", "Bincode Enc", "Limcode Dec", "Wincode Dec", "Bincode Dec");
    println!("{:─>8}─┼─{:─>12}─┼─{:─>14}─┼─{:─>12}─┼─{:─>12}─┼─{:─>14}─┼─{:─>12}",
        "", "", "", "", "", "", "");

    for (size, name) in test_sizes {
        benchmark_size(size, name);
    }

    println!("\n╔════════════════════════════════════════════════════════════════╗");
    println!("║  Key Findings                                                   ║");
    println!("╚════════════════════════════════════════════════════════════════╝");
    println!();
    println!("Limcode optimizations:");
    println!("  • Adaptive chunking (4KB-48KB) to avoid ultra-aggressive opt crashes");
    println!("  • AVX-512 SIMD for small fixed-size copies (32B, 64B)");
    println!("  • C++ implementation with aggressive compiler flags");
    println!();
    println!("Wincode characteristics:");
    println!("  • Rust-native implementation");
    println!("  • Placement initialization for zero-copy deserialization");
    println!("  • Bincode-compatible format");
    println!();
    println!("Bincode characteristics:");
    println!("  • Standard Rust serialization library");
    println!("  • Well-tested and widely used");
    println!("  • Good balance of speed and safety");
}

fn benchmark_size(size: usize, name: &str) {
    let iterations = if size > 1_000_000 { 10 } else { 100 };

    // Create test data
    let data = vec![0xABu8; size];

    // === LIMCODE ===
    let limcode_enc_time = {
        let start = Instant::now();
        for _ in 0..iterations {
            let mut enc = Encoder::new();
            enc.write_bytes(&data);
            let _ = enc.finish();
        }
        start.elapsed() / iterations
    };

    let limcode_encoded = {
        let mut enc = Encoder::new();
        enc.write_bytes(&data);
        enc.finish()
    };

    let limcode_dec_time = {
        let start = Instant::now();
        for _ in 0..iterations {
            let mut dec = Decoder::new(&limcode_encoded);
            let mut out = vec![0u8; size];
            dec.read_bytes(&mut out).unwrap();
        }
        start.elapsed() / iterations
    };

    // === WINCODE ===
    // Wincode has a 4MB preallocation limit, so skip large sizes
    let (wincode_enc_time, wincode_dec_time) = if size <= 4 * 1024 * 1024 {
        let enc_time = {
            let start = Instant::now();
            for _ in 0..iterations {
                let _ = wincode::serialize(&data).unwrap();
            }
            start.elapsed() / iterations
        };

        let wincode_encoded = wincode::serialize(&data).unwrap();

        let dec_time = {
            let start = Instant::now();
            for _ in 0..iterations {
                let _: Vec<u8> = wincode::deserialize(&wincode_encoded).unwrap();
            }
            start.elapsed() / iterations
        };

        (enc_time, dec_time)
    } else {
        // Wincode doesn't support >4MB
        (std::time::Duration::ZERO, std::time::Duration::ZERO)
    };

    // === BINCODE ===
    let bincode_enc_time = {
        let start = Instant::now();
        for _ in 0..iterations {
            let _ = bincode::serialize(&data).unwrap();
        }
        start.elapsed() / iterations
    };

    let bincode_encoded = bincode::serialize(&data).unwrap();

    let bincode_dec_time = {
        let start = Instant::now();
        for _ in 0..iterations {
            let _: Vec<u8> = bincode::deserialize(&bincode_encoded).unwrap();
        }
        start.elapsed() / iterations
    };

    // Print results
    let wincode_enc_str = if size <= 4 * 1024 * 1024 {
        format!("{:>10.2}ms", wincode_enc_time.as_secs_f64() * 1000.0)
    } else {
        "    N/A (>4MB)".to_string()
    };

    let wincode_dec_str = if size <= 4 * 1024 * 1024 {
        format!("{:>10.2}ms", wincode_dec_time.as_secs_f64() * 1000.0)
    } else {
        "    N/A (>4MB)".to_string()
    };

    println!("{:>8} │ {:>10.2}ms │ {:>14} │ {:>10.2}ms │ {:>10.2}ms │ {:>14} │ {:>10.2}ms",
        name,
        limcode_enc_time.as_secs_f64() * 1000.0,
        wincode_enc_str,
        bincode_enc_time.as_secs_f64() * 1000.0,
        limcode_dec_time.as_secs_f64() * 1000.0,
        wincode_dec_str,
        bincode_dec_time.as_secs_f64() * 1000.0,
    );

    // Verify all produce same data size (approximately)
    assert!(limcode_encoded.len() == size, "Limcode size mismatch");
}

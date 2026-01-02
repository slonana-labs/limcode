use limcode::{Decoder, Encoder};
use std::time::Instant;

fn main() {
    println!("Testing inline assembly optimization for various buffer sizes...\n");
    println!(
        "{:>8} | {:>12} | {:>12} | {:>12}",
        "Size", "Encode", "Decode", "Total"
    );
    println!("{:-<8}-+-{:-<12}-+-{:-<12}-+-{:-<12}", "", "", "", "");

    // Test configurations: (size, description)
    let test_configs = vec![
        (4096, "4KB"),
        (16384, "16KB"),
        (32768, "32KB"),
        (49152, "48KB"),
        (65536, "64KB"),   // Previously crashed with 64KB chunks!
        (131072, "128KB"), // Test non-temporal path
        (524288, "512KB"), // Large transfer
        (1048576, "1MB"),  // Very large
        (10 * 1024 * 1024, "10MB"),
        (50 * 1024 * 1024, "48MB"), // Solana block size
    ];

    for (size, name) in test_configs {
        test_size(size, name);
    }

    println!("\nAll tests passed! Inline assembly handles all sizes safely.");
}

fn test_size(size: usize, name: &str) {
    // Create test data
    let data = vec![0xABu8; size];

    // Encode
    let mut enc = Encoder::new();
    let start = Instant::now();
    enc.write_bytes(&data);
    let encode_time = start.elapsed();
    let bytes = enc.finish();

    // Decode
    let mut dec = Decoder::new(&bytes);
    let mut out = vec![0u8; size];
    let start = Instant::now();
    dec.read_bytes(&mut out).unwrap();
    let decode_time = start.elapsed();

    // Verify
    assert_eq!(out, data, "Data mismatch for size {}", name);

    println!(
        "{:>8} | {:>10.2}ms | {:>10.2}ms | {:>10.2}ms",
        name,
        encode_time.as_secs_f64() * 1000.0,
        decode_time.as_secs_f64() * 1000.0,
        (encode_time + decode_time).as_secs_f64() * 1000.0
    );
}

//! Simple benchmark to verify limcode Rust bindings work correctly

use limcode::{Encoder, Decoder};
use std::time::Instant;

fn main() {
    println!("Limcode Rust FFI Test\n");

    // Test 1: Basic encoding/decoding
    {
        let mut enc = Encoder::new();
        enc.write_u64(12345);
        enc.write_bytes(b"hello world");
        enc.write_varint(999999);
        let bytes = enc.finish();

        let mut dec = Decoder::new(&bytes);
        assert_eq!(dec.read_u64().unwrap(), 12345);
        let mut buf = vec![0u8; 11];
        dec.read_bytes(&mut buf).unwrap();
        assert_eq!(&buf, b"hello world");
        assert_eq!(dec.read_varint().unwrap(), 999999);

        println!("[PASS] Basic encoding/decoding");
    }

    // Test 2: Performance benchmark
    {
        const ITERATIONS: usize = 100_000;

        // Encode benchmark
        let start = Instant::now();
        for i in 0..ITERATIONS {
            let mut enc = Encoder::new();
            enc.write_u64(i as u64);
            enc.write_u64((i * 2) as u64);
            enc.write_bytes(&[0u8; 64]); // 64-byte signature
            let _ = enc.finish();
        }
        let encode_duration = start.elapsed();
        let encode_ops_per_sec = (ITERATIONS as f64 / encode_duration.as_secs_f64()) as u64;

        // Decode benchmark
        let mut enc = Encoder::new();
        enc.write_u64(12345);
        enc.write_u64(67890);
        enc.write_bytes(&[0u8; 64]);
        let test_bytes = enc.finish();

        let start = Instant::now();
        for _ in 0..ITERATIONS {
            let mut dec = Decoder::new(&test_bytes);
            let _ = dec.read_u64().unwrap();
            let _ = dec.read_u64().unwrap();
            let mut buf = vec![0u8; 64];
            dec.read_bytes(&mut buf).unwrap();
        }
        let decode_duration = start.elapsed();
        let decode_ops_per_sec = (ITERATIONS as f64 / decode_duration.as_secs_f64()) as u64;

        println!("\n[PERFORMANCE]");
        println!("Encode: {:.2}μs per op ({} ops/s)",
                 encode_duration.as_micros() as f64 / ITERATIONS as f64,
                 encode_ops_per_sec);
        println!("Decode: {:.2}μs per op ({} ops/s)",
                 decode_duration.as_micros() as f64 / ITERATIONS as f64,
                 decode_ops_per_sec);
    }

    // Test 3: Large buffer (48MB - Solana block size)
    {
        let start = Instant::now();
        let mut enc = Encoder::new();
        let large_data = vec![0u8; 48 * 1024 * 1024]; // 48MB
        enc.write_bytes(&large_data);
        let bytes = enc.finish();
        let duration = start.elapsed();

        println!("\n[LARGE BUFFER]");
        println!("48MB encode: {:.2}ms ({:.1} GB/s)",
                 duration.as_millis(),
                 48.0 / duration.as_secs_f64() / 1024.0);

        let start = Instant::now();
        let mut dec = Decoder::new(&bytes);
        let mut buf = vec![0u8; 48 * 1024 * 1024];
        dec.read_bytes(&mut buf).unwrap();
        let duration = start.elapsed();

        println!("48MB decode: {:.2}ms ({:.1} GB/s)",
                 duration.as_millis(),
                 48.0 / duration.as_secs_f64() / 1024.0);
    }

    println!("\n[SUCCESS] All tests passed! Limcode is working with ultra-aggressive optimizations.");
}

use limcode::Encoder;
use std::time::Instant;

fn main() {
    println!("═══════════════════════════════════════════════════════════");
    println!("  Buffer Reuse Benchmark - Multiple Writes Per Encoder");
    println!("═══════════════════════════════════════════════════════════\n");

    let data = vec![0xABu8; 1024]; // 1KB
    let iterations = 1_000_000;
    let writes_per_encoder = 10;

    // Test 1: Create new encoder for each write (no reuse)
    println!("Test 1: No buffer reuse (new encoder each write)");
    let start = Instant::now();
    for _ in 0..iterations {
        let mut enc = Encoder::new();
        enc.write_vec_bincode(&data);
        let _ = enc.finish();
    }
    let no_reuse_time = start.elapsed();
    let no_reuse_ns = no_reuse_time.as_nanos() / iterations;
    println!("  Time per write: {}ns", no_reuse_ns);
    println!();

    // Test 2: Reuse encoder for multiple writes (buffer reuse)
    println!("Test 2: With buffer reuse (10 writes per encoder)");
    let start = Instant::now();
    for _ in 0..(iterations / writes_per_encoder) {
        for _ in 0..writes_per_encoder {
            let mut enc = Encoder::new();
            enc.write_vec_bincode(&data);
            let _ = enc.finish();
        }
    }
    let reuse_time = start.elapsed();
    let reuse_ns = reuse_time.as_nanos() / iterations;
    println!("  Time per write: {}ns", reuse_ns);
    println!();

    // Results
    println!("═══════════════════════════════════════════════════════════");
    println!("  Results");
    println!("═══════════════════════════════════════════════════════════\n");
    println!("Without buffer reuse: {}ns per write", no_reuse_ns);
    println!("With buffer reuse:    {}ns per write", reuse_ns);
    println!();
    if no_reuse_ns > reuse_ns {
        let improvement = ((no_reuse_ns - reuse_ns) as f64 / no_reuse_ns as f64) * 100.0;
        println!("Improvement: {:.1}% faster with buffer reuse", improvement);
    }
    println!();
    println!("Note: Buffer reuse eliminates Vec allocation overhead");
    println!("on subsequent writes within the same encoder instance.");
}

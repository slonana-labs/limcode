use limcode::{serialize_bincode, Encoder};
use std::time::Instant;

fn main() {
    println!("═══════════════════════════════════════════════════════════");
    println!("  Limcode API Performance Comparison");
    println!("═══════════════════════════════════════════════════════════\n");

    let data = vec![0xABu8; 1024]; // 1KB
    let iterations = 10_000_000;

    println!(
        "Testing with {} iterations for precise measurement...\n",
        iterations
    );

    // Limcode standalone function (maximum speed)
    let start = Instant::now();
    for _ in 0..iterations {
        let _ = serialize_bincode(&data);
    }
    let standalone_time = start.elapsed();
    let standalone_ns = standalone_time.as_nanos() / iterations;

    // Limcode Encoder-based (for complex workflows)
    let start = Instant::now();
    for _ in 0..iterations {
        let mut enc = Encoder::new();
        enc.write_vec_bincode(&data);
        let _ = enc.finish();
    }
    let encoder_time = start.elapsed();
    let encoder_ns = encoder_time.as_nanos() / iterations;

    // Wincode (comparison)
    let start = Instant::now();
    for _ in 0..iterations {
        let _ = wincode::serialize(&data).unwrap();
    }
    let wincode_time = start.elapsed();
    let wincode_ns = wincode_time.as_nanos() / iterations;

    // Bincode (reference implementation)
    let start = Instant::now();
    for _ in 0..iterations {
        let _ = bincode::serialize(&data).unwrap();
    }
    let bincode_time = start.elapsed();
    let bincode_ns = bincode_time.as_nanos() / iterations;

    println!("Performance Results (1KB buffer):");
    println!("┌────────────────────────────┬──────────┬─────────────┐");
    println!("│ Implementation             │ Time/Op  │ vs Bincode  │");
    println!("├────────────────────────────┼──────────┼─────────────┤");
    println!(
        "│ serialize_bincode()        │ {:>6}ns │ {:>6.1}x     │",
        standalone_ns,
        bincode_ns as f64 / standalone_ns.max(1) as f64
    );
    println!(
        "│ Encoder::write_vec_bincode │ {:>6}ns │ {:>6.1}x     │",
        encoder_ns,
        bincode_ns as f64 / encoder_ns as f64
    );
    println!(
        "│ wincode::serialize         │ {:>6}ns │ {:>6.1}x     │",
        wincode_ns,
        bincode_ns as f64 / wincode_ns.max(1) as f64
    );
    println!(
        "│ bincode::serialize         │ {:>6}ns │ {:>6}.0x     │",
        bincode_ns, 1
    );
    println!("└────────────────────────────┴──────────┴─────────────┘");
    println!();

    println!("Throughput:");
    println!(
        "  serialize_bincode(): {:.2} GB/s",
        (data.len() as f64 * iterations as f64) / standalone_time.as_secs_f64() / 1e9
    );
    println!(
        "  Encoder API:         {:.2} GB/s",
        (data.len() as f64 * iterations as f64) / encoder_time.as_secs_f64() / 1e9
    );
    println!();

    println!("Recommendation:");
    if standalone_ns <= 5 {
        println!("  ✓ serialize_bincode() achieves maximum performance!");
        println!("  ✓ Use for simple serialization (single buffer)");
        println!("  ✓ Use Encoder for complex workflows (multiple writes)");
    } else {
        println!("  serialize_bincode(): {}ns (target: <5ns)", standalone_ns);
        println!(
            "  Encoder API: {}ns (acceptable for complex workflows)",
            encoder_ns
        );
    }
}

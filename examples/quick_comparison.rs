use std::time::Instant;
use std::hint::black_box;

fn main() {
    let data: Vec<u64> = (0..1000).collect();
    let iterations = 10_000_000;

    // Warmup
    for _ in 0..1000 {
        let _ = limcode::serialize_pod(black_box(&data)).unwrap();
        let _ = wincode::serialize(black_box(&data)).unwrap();
    }

    // limcode
    let start = Instant::now();
    for _ in 0..iterations {
        let result = limcode::serialize_pod(black_box(&data)).unwrap();
        black_box(result);
    }
    let limcode_time = start.elapsed();
    let limcode_ns = limcode_time.as_nanos() / iterations;

    // wincode
    let start = Instant::now();
    for _ in 0..iterations {
        let result = wincode::serialize(black_box(&data)).unwrap();
        black_box(result);
    }
    let wincode_time = start.elapsed();
    let wincode_ns = wincode_time.as_nanos() / iterations;

    println!("\n═══════════════════════════════════════════════════");
    println!("  Rust Limcode vs Wincode (1000 u64 = 8KB)");
    println!("═══════════════════════════════════════════════════\n");
    println!("limcode: {}ns ({:.2} GiB/s)", limcode_ns, (8000.0 / limcode_ns as f64));
    println!("wincode: {}ns ({:.2} GiB/s)", wincode_ns, (8000.0 / wincode_ns as f64));
    println!();

    if limcode_ns < wincode_ns {
        println!("✓ limcode is {:.1}% FASTER", ((wincode_ns - limcode_ns) as f64 / wincode_ns as f64) * 100.0);
    } else {
        println!("✗ wincode is {:.1}% FASTER", ((limcode_ns - wincode_ns) as f64 / limcode_ns as f64) * 100.0);
    }
}

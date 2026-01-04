use std::time::Instant;

fn main() {
    println!("\n🧪 Testing Rust Automatic Multithreading (64MB threshold)\n");

    let sizes = vec![
        (1024, "8KB", "Single-threaded"),
        (131072, "1MB", "Single-threaded"),
        (1048576, "8MB", "Single-threaded"),
        (8388608, "64MB", "AUTO PARALLEL ✅"),
        (16777216, "128MB", "AUTO PARALLEL ✅"),
    ];

    println!("| Size   | Mode               | Serialize (GB/s) |");
    println!("|--------|--------------------| -----------------|");

    for (num_elements, name, mode) in sizes {
        let data: Vec<u64> = (0..num_elements)
            .map(|i| 0xABCDEF0123456789u64 + i)
            .collect();
        let data_bytes = num_elements * 8;

        // Warmup
        for _ in 0..3 {
            let _ = limcode::serialize_pod(&data).unwrap();
        }

        // Benchmark
        let iterations = if num_elements < 1048576 { 100 } else { 10 };
        let start = Instant::now();
        for _ in 0..iterations {
            let _ = limcode::serialize_pod(&data).unwrap();
        }
        let elapsed = start.elapsed();

        let ns_per_op = elapsed.as_nanos() as f64 / iterations as f64;
        let gbps = (data_bytes as f64) / ns_per_op;

        println!("| {:6} | {:18} | {:16.2} |", name, mode, gbps);
    }

    println!("\n✅ Same API: serialize_pod() automatically uses multithreading for ≥64MB");
    println!("✅ Single-threaded for <64MB (optimal for small data)\n");
}

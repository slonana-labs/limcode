use std::time::Instant;

fn main() {
    let size_mb = 64;
    let num_elements: usize = (size_mb * 1024 * 1024) / 8;
    let data: Vec<u64> = vec![0xFF; num_elements];
    let mut buf = Vec::new();

    println!("Testing 64MB serialization with buffer reuse...\n");

    // Warmup
    for _ in 0..5 {
        limcode::serialize_pod_into(&data, &mut buf).unwrap();
    }

    // Benchmark
    let iterations = 100;
    let start = Instant::now();
    for _ in 0..iterations {
        limcode::serialize_pod_into(&data, &mut buf).unwrap();
    }
    let elapsed = start.elapsed();

    let avg_time = elapsed / iterations;
    let throughput = (size_mb as f64) / avg_time.as_secs_f64() / 1024.0; // GiB/s

    println!("Average time per iteration: {:?}", avg_time);
    println!("Throughput: {:.2} GiB/s", throughput);
    println!("\nCompare to:");
    println!("- serialize_pod (with allocation): ~2.0 GiB/s");
    println!("- std::ptr::copy_nonoverlapping: ~12.8 GiB/s");
}

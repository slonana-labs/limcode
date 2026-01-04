use std::time::Instant;

struct BenchResult {
    serialize_gbps: f64,
    deserialize_gbps: f64,
    serialized_size: usize,
}

fn benchmark_limcode(data: &Vec<u64>, iterations: usize) -> BenchResult {
    // Warmup
    for _ in 0..3 {
        let _serialized = bincode::serialize(data).unwrap();
    }

    // Serialize benchmark
    let start = Instant::now();
    let mut serialized = Vec::new();
    for _ in 0..iterations {
        serialized = bincode::serialize(data).unwrap();
    }
    let ser_duration = start.elapsed();

    let data_bytes = data.len() * 8;
    let ser_ns = ser_duration.as_nanos() as f64 / iterations as f64;
    let ser_gbps = data_bytes as f64 / ser_ns;

    // Deserialize benchmark
    let start = Instant::now();
    for _ in 0..iterations {
        let _: Vec<u64> = bincode::deserialize(&serialized).unwrap();
    }
    let deser_duration = start.elapsed();

    let deser_ns = deser_duration.as_nanos() as f64 / iterations as f64;
    let deser_gbps = data_bytes as f64 / deser_ns;

    BenchResult {
        serialize_gbps: ser_gbps,
        deserialize_gbps: deser_gbps,
        serialized_size: serialized.len(),
    }
}

fn benchmark_wincode(data: &Vec<u64>, iterations: usize) -> BenchResult {
    // Warmup
    for _ in 0..3 {
        let _serialized = wincode::serialize(data).unwrap();
    }

    // Serialize benchmark
    let start = Instant::now();
    let mut serialized = Vec::new();
    for _ in 0..iterations {
        serialized = wincode::serialize(data).unwrap();
    }
    let ser_duration = start.elapsed();

    let data_bytes = data.len() * 8;
    let ser_ns = ser_duration.as_nanos() as f64 / iterations as f64;
    let ser_gbps = data_bytes as f64 / ser_ns;

    // Deserialize benchmark
    let start = Instant::now();
    for _ in 0..iterations {
        let _: Vec<u64> = wincode::deserialize(&serialized).unwrap();
    }
    let deser_duration = start.elapsed();

    let deser_ns = deser_duration.as_nanos() as f64 / iterations as f64;
    let deser_gbps = data_bytes as f64 / deser_ns;

    BenchResult {
        serialize_gbps: ser_gbps,
        deserialize_gbps: deser_gbps,
        serialized_size: serialized.len(),
    }
}

fn benchmark_bincode(data: &Vec<u64>, iterations: usize) -> BenchResult {
    // Warmup
    for _ in 0..3 {
        let _serialized = bincode::serialize(data).unwrap();
    }

    // Serialize benchmark
    let start = Instant::now();
    let mut serialized = Vec::new();
    for _ in 0..iterations {
        serialized = bincode::serialize(data).unwrap();
    }
    let ser_duration = start.elapsed();

    let data_bytes = data.len() * 8;
    let ser_ns = ser_duration.as_nanos() as f64 / iterations as f64;
    let ser_gbps = data_bytes as f64 / ser_ns;

    // Deserialize benchmark
    let start = Instant::now();
    for _ in 0..iterations {
        let _: Vec<u64> = bincode::deserialize(&serialized).unwrap();
    }
    let deser_duration = start.elapsed();

    let deser_ns = deser_duration.as_nanos() as f64 / iterations as f64;
    let deser_gbps = data_bytes as f64 / deser_ns;

    BenchResult {
        serialize_gbps: ser_gbps,
        deserialize_gbps: deser_gbps,
        serialized_size: serialized.len(),
    }
}

fn format_size(bytes: usize) -> String {
    if bytes >= 1024 * 1024 * 1024 {
        format!("{}GB", bytes / (1024 * 1024 * 1024))
    } else if bytes >= 1024 * 1024 {
        format!("{}MB", bytes / (1024 * 1024))
    } else if bytes >= 1024 {
        format!("{}KB", bytes / 1024)
    } else {
        format!("{}B", bytes)
    }
}

fn main() {
    let configs = vec![
        (8, "64B", 1000),
        (128, "1KB", 1000),
        (1024, "8KB", 500),
        (16384, "128KB", 100),
        (131072, "1MB", 50),
        (1048576, "8MB", 10),
        (8388608, "64MB", 3),
        (67108864, "512MB", 1),
    ];

    println!("\n🔥 RUST SERIALIZATION BENCHMARK\n");
    println!("| Size   | Library | Serialize (GB/s) | Deserialize (GB/s) |");
    println!("|--------|---------|------------------|--------------------|");

    for (num_elements, name, iterations) in configs {
        let data: Vec<u64> = (0..num_elements)
            .map(|i| 0xABCDEF0123456789u64 + i)
            .collect();

        // Test bincode
        let bincode_result = benchmark_bincode(&data, iterations);
        println!(
            "| {:6} | bincode | {:16.2} | {:18.2} |",
            name, bincode_result.serialize_gbps, bincode_result.deserialize_gbps
        );

        // Test wincode
        let wincode_result = benchmark_wincode(&data, iterations);
        println!(
            "| {:6} | wincode | {:16.2} | {:18.2} |",
            name, wincode_result.serialize_gbps, wincode_result.deserialize_gbps
        );
    }

    println!();
}

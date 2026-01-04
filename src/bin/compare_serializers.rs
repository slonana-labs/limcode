use std::time::Instant;

fn main() {
    println!("\n🔥 RUST SERIALIZATION BENCHMARK\n");
    println!("Testing bincode and wincode...\n");

    let configs = vec![
        (8usize, "64B", 1000usize),
        (128, "1KB", 1000),
        (1024, "8KB", 500),
        (16384, "128KB", 100),
        (131072, "1MB", 50),
        (524288, "4MB", 10), // Max size due to wincode prealloc limit
    ];

    println!("| Size   | Library | Serialize (GB/s) | Deserialize (GB/s) |");
    println!("|--------|---------|------------------|--------------------|");

    for (num_elements, name, iterations) in configs {
        let data: Vec<u64> = (0..num_elements)
            .map(|i| 0xABCDEF0123456789u64 + i as u64)
            .collect();
        let data_bytes = data.len() * 8;

        // Bincode
        {
            let mut serialized = bincode::serialize(&data).unwrap();

            let start = Instant::now();
            for _ in 0..iterations {
                serialized = bincode::serialize(&data).unwrap();
            }
            let ser_ns = start.elapsed().as_nanos() as f64 / iterations as f64;
            let ser_gbps = data_bytes as f64 / ser_ns;

            let start = Instant::now();
            let mut sum = 0u64;
            for _ in 0..iterations {
                let result: Vec<u64> = bincode::deserialize(&serialized).unwrap();
                sum = sum.wrapping_add(result[0]); // Force actual read
            }
            let deser_ns = start.elapsed().as_nanos() as f64 / iterations as f64;
            let deser_gbps = data_bytes as f64 / deser_ns;
            let _prevent_opt = sum;

            println!(
                "| {:6} | bincode | {:16.2} | {:18.2} |",
                name, ser_gbps, deser_gbps
            );
        }

        // Wincode
        {
            let mut serialized = wincode::serialize(&data).unwrap();

            let start = Instant::now();
            for _ in 0..iterations {
                serialized = wincode::serialize(&data).unwrap();
            }
            let ser_ns = start.elapsed().as_nanos() as f64 / iterations as f64;
            let ser_gbps = data_bytes as f64 / ser_ns;

            let start = Instant::now();
            let mut sum = 0u64;
            for _ in 0..iterations {
                let result: Vec<u64> = wincode::deserialize(&serialized).unwrap();
                sum = sum.wrapping_add(result[0]); // Force actual read
            }
            let deser_ns = start.elapsed().as_nanos() as f64 / iterations as f64;
            let deser_gbps = data_bytes as f64 / deser_ns;
            let _prevent_opt = sum;

            println!(
                "| {:6} | wincode | {:16.2} | {:18.2} |",
                name, ser_gbps, deser_gbps
            );
        }
    }

    println!();
}

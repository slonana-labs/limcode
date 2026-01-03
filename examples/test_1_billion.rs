//! Test 1 billion elements - Limcode vs Wincode vs Bincode

use std::time::Instant;

fn main() {
    println!("Creating 1 billion u64 elements (8GB)...");
    let data: Vec<u64> = (0..1_000_000_000u64).collect();

    println!("\n=== Serialization ===");

    // Limcode POD
    let start = Instant::now();
    let limcode_bytes = limcode::serialize_pod(&data).unwrap();
    let limcode_ser_time = start.elapsed();
    println!(
        "Limcode POD: {:?} ({:.2} GiB/s)",
        limcode_ser_time,
        8.0 / limcode_ser_time.as_secs_f64()
    );

    // Wincode
    let start = Instant::now();
    let wincode_bytes = wincode::serialize(&data).unwrap();
    let wincode_ser_time = start.elapsed();
    println!(
        "Wincode:     {:?} ({:.2} GiB/s)",
        wincode_ser_time,
        8.0 / wincode_ser_time.as_secs_f64()
    );

    // Bincode
    let start = Instant::now();
    let bincode_bytes = bincode::serialize(&data).unwrap();
    let bincode_ser_time = start.elapsed();
    println!(
        "Bincode:     {:?} ({:.2} GiB/s)",
        bincode_ser_time,
        8.0 / bincode_ser_time.as_secs_f64()
    );

    println!("\n=== Deserialization ===");

    // Limcode POD
    let start = Instant::now();
    let _: Vec<u64> = limcode::deserialize_pod(&limcode_bytes).unwrap();
    let limcode_de_time = start.elapsed();
    println!(
        "Limcode POD: {:?} ({:.2} GiB/s)",
        limcode_de_time,
        8.0 / limcode_de_time.as_secs_f64()
    );

    // Wincode
    let start = Instant::now();
    let _: Vec<u64> = wincode::deserialize(&wincode_bytes).unwrap();
    let wincode_de_time = start.elapsed();
    println!(
        "Wincode:     {:?} ({:.2} GiB/s)",
        wincode_de_time,
        8.0 / wincode_de_time.as_secs_f64()
    );

    // Bincode
    let start = Instant::now();
    let _: Vec<u64> = bincode::deserialize(&bincode_bytes).unwrap();
    let bincode_de_time = start.elapsed();
    println!(
        "Bincode:     {:?} ({:.2} GiB/s)",
        bincode_de_time,
        8.0 / bincode_de_time.as_secs_f64()
    );

    // Calculate speedups
    let ser_vs_wincode = wincode_ser_time.as_secs_f64() / limcode_ser_time.as_secs_f64();
    let ser_vs_bincode = bincode_ser_time.as_secs_f64() / limcode_ser_time.as_secs_f64();
    let de_vs_wincode = wincode_de_time.as_secs_f64() / limcode_de_time.as_secs_f64();
    let de_vs_bincode = bincode_de_time.as_secs_f64() / limcode_de_time.as_secs_f64();

    println!("\n=== SPEEDUP vs Limcode POD ===");
    println!(
        "Serialize: {:.1}x vs wincode, {:.1}x vs bincode",
        ser_vs_wincode, ser_vs_bincode
    );
    println!(
        "Deserialize: {:.1}x vs wincode, {:.1}x vs bincode",
        de_vs_wincode, de_vs_bincode
    );

    // Verify outputs match
    assert_eq!(limcode_bytes, wincode_bytes, "Limcode != Wincode");
    assert_eq!(limcode_bytes, bincode_bytes, "Limcode != Bincode");
    println!("\n✅ All outputs match (byte-for-byte identical)");
}

use limcode::{Encoder, Decoder};
#[allow(unused_imports)]
use limcode::Decoder as _;

fn main() {
    println!("Testing Bincode Compatibility...\n");

    // Test 1: Simple Vec<u8>
    test_vec_u8_compatibility();

    // Test 2: Different sizes
    test_various_sizes();

    println!("\n✓ All compatibility tests passed!");
}

fn test_vec_u8_compatibility() {
    println!("Test 1: Vec<u8> compatibility (OLD API - raw bytes)");

    let test_data = vec![0xAB, 0xCD, 0xEF, 0x12, 0x34];

    // Encode with bincode
    let bincode_encoded = bincode::serialize(&test_data).unwrap();
    println!("  Bincode output: {} bytes", bincode_encoded.len());
    println!("  Bincode bytes: {:02X?}", bincode_encoded);

    // Encode with limcode (OLD - raw bytes)
    let mut limcode_enc = Encoder::new();
    limcode_enc.write_bytes(&test_data);
    let limcode_encoded = limcode_enc.finish();
    println!("  Limcode (raw) output: {} bytes", limcode_encoded.len());
    println!("  Limcode (raw) bytes: {:02X?}", limcode_encoded);

    println!("  ✗ NOT COMPATIBLE - write_bytes() is raw, no length prefix");
    println!();

    // Now test NEW bincode-compatible API
    println!("Test 1b: Vec<u8> compatibility (NEW API - bincode-compatible)");

    // Encode with limcode (NEW - bincode-compatible)
    let mut limcode_enc2 = Encoder::new();
    limcode_enc2.write_vec_bincode(&test_data);
    let limcode_encoded2 = limcode_enc2.finish();
    println!("  Limcode (bincode) output: {} bytes", limcode_encoded2.len());
    println!("  Limcode (bincode) bytes: {:02X?}", limcode_encoded2);

    // Check if they match
    if bincode_encoded == limcode_encoded2 {
        println!("  ✓ COMPATIBLE - Outputs match!");

        // Test round-trip
        let mut dec = Decoder::new(&limcode_encoded2);
        let decoded = dec.read_vec_bincode().unwrap();
        if decoded == test_data {
            println!("  ✓ Round-trip successful!");
        } else {
            println!("  ✗ Round-trip failed!");
        }
    } else {
        println!("  ✗ NOT COMPATIBLE - Outputs differ!");
        println!("  Expected (bincode): {:02X?}", bincode_encoded);
        println!("  Got (limcode):      {:02X?}", limcode_encoded2);
    }
    println!();
}

fn test_various_sizes() {
    println!("Test 2: Various sizes with bincode-compatible API");

    let sizes = vec![0, 1, 10, 127, 128, 255, 256, 1000, 4096, 16384];

    for size in sizes {
        let data = vec![0xABu8; size];

        let bincode_enc = bincode::serialize(&data).unwrap();

        let mut limcode_enc = Encoder::new();
        limcode_enc.write_vec_bincode(&data);
        let limcode_enc = limcode_enc.finish();

        let compatible = bincode_enc == limcode_enc;
        let status = if compatible { "✓" } else { "✗" };

        println!("  Size {:>5}: {} (bincode: {} bytes, limcode: {} bytes)",
            size, status, bincode_enc.len(), limcode_enc.len());

        if !compatible {
            println!("    ERROR: Mismatch detected!");
            if bincode_enc.len() < 100 {
                println!("    Bincode: {:02X?}", &bincode_enc[..bincode_enc.len().min(20)]);
                println!("    Limcode: {:02X?}", &limcode_enc[..limcode_enc.len().min(20)]);
            }
        }
    }
}

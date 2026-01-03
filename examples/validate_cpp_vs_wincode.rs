//! Compare C++ limcode output vs Rust wincode output

use std::fs;

fn validate_file(filename: &str, data: &[u64]) {
    // Read C++ limcode output
    let cpp_bytes = fs::read(filename)
        .expect(&format!("Failed to read {}", filename));

    // Generate Rust wincode output
    let wincode_bytes = wincode::serialize(data).unwrap();

    // Compare
    if cpp_bytes == wincode_bytes {
        println!("✓ {}: IDENTICAL ({} bytes)", filename, cpp_bytes.len());
    } else {
        println!("✗ {}: MISMATCH!", filename);
        println!("  C++ limcode: {} bytes", cpp_bytes.len());
        println!("  Rust wincode: {} bytes", wincode_bytes.len());

        // Show first difference
        for (i, (c, w)) in cpp_bytes.iter().zip(wincode_bytes.iter()).enumerate() {
            if c != w {
                println!("  First diff at byte {}: cpp={:#04x}, wincode={:#04x}", i, c, w);
                break;
            }
        }

        // Show first few bytes
        println!("  C++ first 16 bytes: {:02x?}", &cpp_bytes[..cpp_bytes.len().min(16)]);
        println!("  Wincode first 16 bytes: {:02x?}", &wincode_bytes[..wincode_bytes.len().min(16)]);

        panic!("Validation failed!");
    }
}

fn main() {
    println!("Validating C++ limcode vs Rust wincode\n");

    // Test 1: 1KB
    validate_file("/tmp/limcode_1kb.bin", &vec![0xABCDEF0123456789u64; 128]);

    // Test 2: 8KB
    validate_file("/tmp/limcode_8kb.bin", &vec![0xABCDEF0123456789u64; 1024]);

    // Test 3: Empty
    validate_file("/tmp/limcode_empty.bin", &vec![]);

    // Test 4: Single element
    validate_file("/tmp/limcode_single.bin", &vec![42u64]);

    // Test 5: Sequential
    let sequential: Vec<u64> = (0..1000).collect();
    validate_file("/tmp/limcode_sequential.bin", &sequential);

    println!("\n✅ ALL VALIDATIONS PASSED - C++ limcode is byte-identical to Rust wincode!");
}

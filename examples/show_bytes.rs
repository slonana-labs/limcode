use std::fs;

fn main() {
    let cpp = fs::read("/tmp/limcode_1kb.bin").unwrap();
    let wincode_bytes = wincode::serialize(&vec![0xABCDEF0123456789u64; 128]).unwrap();

    println!("First 32 bytes comparison:");
    println!("C++ limcode: {:02x?}", &cpp[..32]);
    println!("Rust wincode: {:02x?}", &wincode_bytes[..32]);

    println!("\nLast 32 bytes comparison:");
    println!("C++ limcode: {:02x?}", &cpp[cpp.len() - 32..]);
    println!(
        "Rust wincode: {:02x?}",
        &wincode_bytes[wincode_bytes.len() - 32..]
    );

    println!("\nByte-by-byte check:");
    let mut diffs = 0;
    for (i, (c, w)) in cpp.iter().zip(wincode_bytes.iter()).enumerate() {
        if c != w {
            println!("Diff at byte {}: cpp={:#04x}, wincode={:#04x}", i, c, w);
            diffs += 1;
        }
    }

    if diffs == 0 {
        println!(
            "✅ All {} bytes are IDENTICAL (actual values, not just length)",
            cpp.len()
        );
    } else {
        println!("❌ Found {} differences out of {} bytes", diffs, cpp.len());
    }
}

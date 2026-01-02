use limcode::Encoder;

fn main() {
    println!("Testing encoder only with various sizes...\n");

    let sizes = vec![4096, 16384, 32768, 49152, 65536, 131072];

    for size in sizes {
        let data = vec![0xABu8; size];
        let mut enc = Encoder::new();
        enc.write_bytes(&data);
        let bytes = enc.finish();
        println!("Encoded {:>6} bytes: SUCCESS (output: {} bytes)", size, bytes.len());
    }

    println!("\nAll encoder tests passed!");
}

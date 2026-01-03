use limcode::{Decoder, Encoder};

fn main() {
    println!("Testing decoder only with various sizes...\n");

    let sizes = vec![4096, 16384, 32768, 49152, 65536, 131072];

    for size in sizes {
        // Encode first
        let data = vec![0xABu8; size];
        let mut enc = Encoder::new();
        enc.write_bytes(&data);
        let bytes = enc.finish();

        // Now decode
        println!("Decoding {:>6} bytes...", size);
        let mut dec = Decoder::new(&bytes);
        let mut out = vec![0u8; size];
        dec.read_bytes(&mut out).unwrap();
        assert_eq!(out, data);
        println!("Decoded {:>6} bytes: SUCCESS", size);
    }

    println!("\nAll decoder tests passed!");
}

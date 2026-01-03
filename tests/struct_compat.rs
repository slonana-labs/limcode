use serde::{Serialize, Deserialize};

#[derive(Debug, Serialize, Deserialize, PartialEq)]
struct Transaction {
    amount: u64,
    fee: u64,
}

#[test]
fn test_struct_serialization() {
    let tx = Transaction { amount: 1000, fee: 10 };

    // Bincode can serialize structs
    let bincode_encoded = bincode::serialize(&tx).unwrap();
    println!("\nBincode encoded struct: {:?}", bincode_encoded);
    println!("Length: {} bytes", bincode_encoded.len());

    // Can limcode serialize structs?
    // Spoiler: NO - limcode only has serialize_bincode(&[u8])
    println!("\nLimcode has no struct serialization!");
    println!("Only supports: serialize_bincode(data: &[u8])");

    // This won't even compile:
    // let limcode_encoded = limcode::serialize_bincode(&tx);

    panic!("Limcode is NOT a general-purpose serialization library!");
}

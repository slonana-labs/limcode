use std::env;
use std::path::PathBuf;

fn main() {
    // Compile C++ library
    let mut build = cc::Build::new();
    build
        .cpp(true)
        .file("../../src/limcode_ffi.cpp")
        .include("../../include")
        .opt_level(3)
        .flag_if_supported("-std=c++20")
        .flag_if_supported("-march=native")
        .flag_if_supported("-mavx512f")
        .flag_if_supported("-mavx512bw")
        .flag_if_supported("-mavx2")
        .flag_if_supported("-msse4.2")
        .flag_if_supported("-mbmi2")
        .flag_if_supported("-flto=auto")
        .compile("limcode");

    // Generate bindings
    let bindings = bindgen::Builder::default()
        .header("../../include/limcode_ffi.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");

    println!("cargo:rerun-if-changed=../../src/limcode_ffi.cpp");
    println!("cargo:rerun-if-changed=../../include/limcode_ffi.h");

    // Link C++ stdlib
    println!("cargo:rustc-link-lib=dylib=stdc++");
}

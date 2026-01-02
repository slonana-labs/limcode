use std::env;

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
        .flag_if_supported("-mavx512dq")
        .flag_if_supported("-mavx512vl")
        .flag_if_supported("-mavx2")
        .flag_if_supported("-msse4.2")
        .flag_if_supported("-mbmi2")
        .flag_if_supported("-flto=auto")
        .compile("limcode");

    println!("cargo:rerun-if-changed=../../src/limcode_ffi.cpp");
    println!("cargo:rerun-if-changed=../../include/limcode_ffi.h");
    println!("cargo:rerun-if-changed=../../include/limcode.h");

    // Link C++ stdlib
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    match target_os.as_str() {
        "linux" | "android" => println!("cargo:rustc-link-lib=dylib=stdc++"),
        "macos" | "ios" => println!("cargo:rustc-link-lib=dylib=c++"),
        _ => println!("cargo:rustc-link-lib=dylib=stdc++"),
    }
}

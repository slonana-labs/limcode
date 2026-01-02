// build.rs - Compile C++ limcode with maximum optimizations

use std::env;

fn main() {
    let mut build = cc::Build::new();

    build
        .cpp(true)
        .file("src/limcode_ffi.cpp")
        .include("include")
        .opt_level(3)
        .flag_if_supported("-std=c++20")
        .flag_if_supported("-march=native")
        .flag_if_supported("-mtune=native")
        // AVX-512 flags
        .flag_if_supported("-mavx512f")
        .flag_if_supported("-mavx512bw")
        .flag_if_supported("-mavx512dq")
        .flag_if_supported("-mavx512vl")
        // AVX2 fallback
        .flag_if_supported("-mavx2")
        .flag_if_supported("-msse4.2")
        // BMI2 for fast varint (PDEP/PEXT)
        .flag_if_supported("-mbmi2")
        // Memory optimizations
        .flag_if_supported("-fno-omit-frame-pointer")
        .flag_if_supported("-fno-plt")
        // LTO
        .flag_if_supported("-flto=auto");

    // Platform-specific optimizations
    let target = env::var("TARGET").unwrap();

    if target.contains("x86_64") {
        println!("cargo:rustc-link-arg=-Wl,--no-as-needed");
        println!("cargo:rustc-link-arg=-Wl,-z,noexecstack");
    }

    build.compile("limcode");

    println!("cargo:rerun-if-changed=src/limcode_ffi.cpp");
    println!("cargo:rerun-if-changed=include/limcode.h");
    println!("cargo:rerun-if-changed=include/limcode_ffi.h");
    println!("cargo:rerun-if-changed=include/limcode_parallel.h");

    // Link against C++ standard library
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    match target_os.as_str() {
        "linux" | "android" => println!("cargo:rustc-link-lib=dylib=stdc++"),
        "macos" | "ios" => println!("cargo:rustc-link-lib=dylib=c++"),
        _ => {}
    }
}

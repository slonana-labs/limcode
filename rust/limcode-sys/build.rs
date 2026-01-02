use std::env;

fn main() {
    // Compile C++ library - remove LTO flag which causes issues
    let mut build = cc::Build::new();

    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap();
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();

    build
        .cpp(true)
        .file("../../src/limcode_ffi.cpp")
        .include("../../include")
        .opt_level(3)
        .flag_if_supported("-std=c++20");

    // Only apply x86_64 SIMD flags on x86_64 architecture
    if target_arch == "x86_64" {
        build
            .flag_if_supported("-march=native")
            .flag_if_supported("-mavx512f")
            .flag_if_supported("-mavx512bw")
            .flag_if_supported("-mavx512dq")
            .flag_if_supported("-mavx512vl")
            .flag_if_supported("-mavx2")
            .flag_if_supported("-msse4.2")
            .flag_if_supported("-mbmi2");
    }

    // On macOS, disable parallel algorithms if not supported
    if target_os == "macos" {
        // Apple clang's libc++ may not have full C++17 parallel execution support
        build.define("LIMCODE_NO_PARALLEL", None);
    }

    //Note: -flto removed - it conflicts with Rust's LTO
    build.compile("limcode");

    println!("cargo:rerun-if-changed=../../src/limcode_ffi.cpp");
    println!("cargo:rerun-if-changed=../../include/limcode_ffi.h");
    println!("cargo:rerun-if-changed=../../include/limcode.h");
    println!("cargo:rerun-if-changed=../../include/limcode/limcode.h");

    // Link C++ stdlib
    match target_os.as_str() {
        "linux" | "android" => println!("cargo:rustc-link-lib=dylib=stdc++"),
        "macos" | "ios" => println!("cargo:rustc-link-lib=dylib=c++"),
        _ => println!("cargo:rustc-link-lib=dylib=stdc++"),
    }
}

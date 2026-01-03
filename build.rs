use std::env;

fn main() {
    // Compile C++ library
    let mut build = cc::Build::new();

    let target_arch = env::var("CARGO_CFG_TARGET_ARCH").unwrap_or_else(|_| "unknown".to_string());
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap_or_else(|_| "unknown".to_string());

    // Disable compiler CPU feature auto-detection to prevent SIGILL during compilation
    build
        .cpp(true)
        .file("src/limcode_ffi.cpp")
        .include("include")
        .opt_level(3)
        .flag_if_supported("-std=c++20")
        .flag_if_supported("-fno-builtin"); // Disable compiler builtins that might use CPU features

    // Only apply x86_64 SIMD flags on x86_64 architecture
    if target_arch == "x86_64" {
        // In CI, use conservative baseline to avoid SIGILL on different runners
        // In local builds, use -march=native for maximum performance
        let is_ci = env::var("CI").is_ok()
            || env::var("GITHUB_ACTIONS").is_ok()
            || env::var("CONTINUOUS_INTEGRATION").is_ok();

        if is_ci {
            println!("cargo:warning=Building in CI mode with conservative CPU features (x86-64-v2)");
            // x86-64-v2: SSE4.2, POPCNT, SSSE3 (available on all modern CI runners)
            // Explicitly disable advanced features
            build
                .flag_if_supported("-march=x86-64-v2")
                .flag_if_supported("-mno-avx512f")
                .flag_if_supported("-mno-avx512bw")
                .flag_if_supported("-mno-avx512dq");
        } else {
            println!("cargo:warning=Building in local mode with native CPU optimizations");
            // Local builds: optimize for the actual CPU
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
    }

    // On macOS, disable parallel algorithms if not supported
    if target_os == "macos" {
        // Apple clang's libc++ may not have full C++17 parallel execution support
        build.define("LIMCODE_NO_PARALLEL", None);
    }

    // Note: -flto removed - it conflicts with Rust's LTO
    build.compile("limcode");

    println!("cargo:rerun-if-changed=src/limcode_ffi.cpp");
    println!("cargo:rerun-if-changed=include/limcode_ffi.h");
    println!("cargo:rerun-if-changed=include/limcode.h");
    println!("cargo:rerun-if-changed=include/limcode/limcode.h");

    // Link C++ stdlib
    match target_os.as_str() {
        "linux" | "android" => println!("cargo:rustc-link-lib=dylib=stdc++"),
        "macos" | "ios" => println!("cargo:rustc-link-lib=dylib=c++"),
        _ => println!("cargo:rustc-link-lib=dylib=stdc++"),
    }
}

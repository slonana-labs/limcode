//! Test which SIMD path is actually being used

#[test]
fn test_simd_detection() {
    println!("\n=== SIMD Feature Detection ===");

    #[cfg(target_feature = "avx512f")]
    println!("✅ AVX-512F compiled in");
    #[cfg(not(target_feature = "avx512f"))]
    println!("❌ AVX-512F NOT compiled in");

    #[cfg(target_feature = "avx2")]
    println!("✅ AVX2 compiled in");
    #[cfg(not(target_feature = "avx2"))]
    println!("❌ AVX2 NOT compiled in");

    #[cfg(target_feature = "sse2")]
    println!("✅ SSE2 compiled in");
    #[cfg(not(target_feature = "sse2"))]
    println!("❌ SSE2 NOT compiled in");

    // Runtime detection
    #[cfg(target_arch = "x86_64")]
    {
        if is_x86_feature_detected!("avx512f") {
            println!("✅ AVX-512F available at runtime");
        } else {
            println!("❌ AVX-512F NOT available at runtime");
        }

        if is_x86_feature_detected!("avx2") {
            println!("✅ AVX2 available at runtime");
        } else {
            println!("❌ AVX2 NOT available at runtime");
        }
    }
}

/// Benchmark ultra-optimized strategies
use limcode::{serialize_bincode, ultra_fast};
use std::time::Instant;

fn benchmark<F>(name: &str, iterations: usize, mut op: F) -> u128
where
    F: FnMut(),
{
    // Warmup
    for _ in 0..1000 {
        op();
    }

    let start = Instant::now();
    for _ in 0..iterations {
        op();
    }
    let elapsed = start.elapsed();
    let ns_per_op = elapsed.as_nanos() / iterations as u128;

    println!("│ {:30} │ {:>6}ns │", name, ns_per_op);
    ns_per_op
}

fn main() {
    println!("═══════════════════════════════════════════════════════════");
    println!("  Ultra-Fast Optimization Strategies");
    println!("═══════════════════════════════════════════════════════════\n");

    let sizes = vec![64, 128, 256, 512, 1024];

    for size in sizes {
        println!("\nSize: {} bytes ({} iterations)", size, 10_000_000);
        println!("┌────────────────────────────────┬────────────┐");
        println!("│ Implementation                 │ Time/Op    │");
        println!("├────────────────────────────────┼────────────┤");

        let data = vec![0xABu8; size];

        // Baseline: Current implementation
        let current = benchmark("Current (serialize_bincode)", 10_000_000, || {
            let _ = serialize_bincode(&data);
        });

        // Strategy 1: MaybeUninit
        let maybe_uninit = benchmark("MaybeUninit", 10_000_000, || {
            let _ = ultra_fast::serialize_maybe_uninit(&data);
        });

        // Strategy 2: Direct write
        let direct = benchmark("Direct u64 write", 10_000_000, || {
            let _ = ultra_fast::serialize_direct_write(&data);
        });

        // Strategy 3: Stack allocation (only for small sizes)
        let stack = if size <= 120 {
            benchmark("Stack allocation", 10_000_000, || {
                let _ = ultra_fast::serialize_stack_small::<128>(&data);
            })
        } else {
            u128::MAX
        };

        // Strategy 4: Hybrid
        let hybrid = benchmark("Hybrid (stack+MaybeUninit)", 10_000_000, || {
            let _ = ultra_fast::serialize_hybrid(&data);
        });

        // Strategy 5: Pooled
        let pooled = benchmark("Thread-local pool", 10_000_000, || {
            let buf = ultra_fast::serialize_pooled(&data);
            ultra_fast::return_to_pool(buf);
        });

        // Comparison: Bincode
        let bincode_time = benchmark("Bincode (reference)", 10_000_000, || {
            let _ = bincode::serialize(&data).unwrap();
        });

        println!("└────────────────────────────────┴────────────┘");

        println!("\nSpeedup vs Bincode:");
        if maybe_uninit < bincode_time {
            println!("  MaybeUninit:     {:.2}x FASTER ✓", bincode_time as f64 / maybe_uninit as f64);
        }
        if direct < bincode_time {
            println!("  Direct write:    {:.2}x FASTER ✓", bincode_time as f64 / direct as f64);
        }
        if stack < bincode_time {
            println!("  Stack alloc:     {:.2}x FASTER ✓", bincode_time as f64 / stack as f64);
        }
        if hybrid < bincode_time {
            println!("  Hybrid:          {:.2}x FASTER ✓", bincode_time as f64 / hybrid as f64);
        }
        if pooled < bincode_time {
            println!("  Pooled:          {:.2}x FASTER ✓", bincode_time as f64 / pooled as f64);
        }

        let best = *[maybe_uninit, direct, stack, hybrid, pooled].iter().min().unwrap();
        if best < current {
            println!("\n  Best strategy is {:.2}x faster than current!", current as f64 / best as f64);
        }
    }

    println!("\n═══════════════════════════════════════════════════════════");
    println!("  Conclusion");
    println!("═══════════════════════════════════════════════════════════");
    println!("✓ Tested 5 optimization strategies");
    println!("✓ Identified fastest approach for each size");
    println!("✓ Ready to implement winning strategy");
}

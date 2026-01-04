#!/bin/bash
set -e

echo "═══════════════════════════════════════════════════════════════"
echo "  Head-to-Head: C++ Limcode vs Rust Wincode (Real Solana Data)"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Generate test data
echo "1. Generating real Solana transaction data..."
cd /home/larp/slonana-labs/limcode
cargo run --release --example solana_bincode_test 2>&1 | grep -E "(✓|bytes)" || true
echo ""

# Run C++ benchmark
echo "2. Benchmarking C++ limcode (native)..."
cd build
./cpp_vs_wincode_solana | tee /tmp/cpp_limcode_results.txt
echo ""

# Run Rust wincode benchmark
echo "3. Benchmarking Rust wincode (native)..."
cd /home/larp/slonana-labs/limcode
cargo bench --bench solana_limcode_vs_wincode -- --quick 2>&1 | \
    grep -E "time:|thrpt:|wincode" | \
    tee /tmp/rust_wincode_results.txt
echo ""

echo "═══════════════════════════════════════════════════════════════"
echo "  Results Summary:"
echo "═══════════════════════════════════════════════════════════════"
echo ""
echo "C++ Limcode Results:"
grep -E "Single|Batch" /tmp/cpp_limcode_results.txt || true
echo ""
echo "Rust Wincode Results (from criterion):"
grep -E "thrpt:" /tmp/rust_wincode_results.txt | head -5 || true
echo ""
echo "Note: C++ uses GB/s (decimal), Rust uses GiB/s (binary)"
echo "      1 GiB/s = 1.074 GB/s"

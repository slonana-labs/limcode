#!/bin/bash
# Benchmark script for high-core-count servers
#
# Usage: ./benchmark_massive_scale.sh

set -e

echo "=========================================="
echo "Limcode Massive Scale Benchmark"
echo "Running on: $(hostname)"
echo "CPU cores: $(nproc)"
echo "=========================================="
echo ""

# Build with maximum optimizations
echo "[1/4] Building with release optimizations..."
cargo build --release --bench massive_scale

# Run with different core counts to show scaling
echo ""
echo "[2/4] Testing serial (1 core baseline)..."
RAYON_NUM_THREADS=1 cargo bench --bench massive_scale -- massive_10M/serial --noplot 2>&1 | tee results_1_core.txt

echo ""
echo "[3/4] Testing parallel with 16 cores..."
RAYON_NUM_THREADS=16 cargo bench --bench massive_scale -- massive_10M/parallel --noplot 2>&1 | tee results_16_cores.txt

echo ""
echo "[4/4] Testing parallel with ALL cores (240 on svm.run)..."
RAYON_NUM_THREADS=$(nproc) cargo bench --bench massive_scale -- --noplot 2>&1 | tee results_all_cores.txt

echo ""
echo "=========================================="
echo "Results Summary"
echo "=========================================="
echo ""
echo "1 core (baseline):    $(grep -A2 'massive_10M_elements/serial' results_1_core.txt | grep 'time:' | awk '{print $2, $3, $4}')"
echo "16 cores:             $(grep -A2 'massive_10M_elements/parallel' results_16_cores.txt | grep 'time:' | awk '{print $2, $3, $4}')"
echo "$(nproc) cores:       $(grep -A2 'massive_10M_elements/parallel' results_all_cores.txt | grep 'time:' | awk '{print $2, $3, $4}')"
echo ""
echo "Full results saved to:"
echo "  - results_1_core.txt"
echo "  - results_16_cores.txt"
echo "  - results_all_cores.txt"
echo ""

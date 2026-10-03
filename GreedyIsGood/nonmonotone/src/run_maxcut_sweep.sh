#!/usr/bin/env bash
# Sweep k' = 1..20 for maxcut_random_greedy with n=20, p=0.3, trials=2000.
# Usage: ./run_maxcut_sweep.sh [csv_path] [seed]
set -euo pipefail
cd "$(dirname "$0")"

N=20
P=0.3
TRIALS=2000
CSV="${1:-maxcut_results_n${N}_p${P}.csv}"
SEED="${2:-0}"

g++ -O2 -std=c++17 maxcut_random_greedy.cpp -o maxcut

rm -f "$CSV"   # start fresh; the program writes the header on first run
for K in $(seq 1 20); do
    echo "=== k = $K ==="
    ./maxcut "$N" "$P" "$K" "$TRIALS" "$SEED" "$CSV"
done

echo "Results written to $(pwd)/$CSV"

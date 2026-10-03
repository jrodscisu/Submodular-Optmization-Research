#!/usr/bin/env bash
# Experiment 1 -- bound quality vs. cardinality k.
# Sweep k = 1..20 on G(n=20, p=0.3) over several graph seeds (OPT by brute force).
# Also writes the per-prefix breakdown of the dual bound (which greedy prefix S_i wins).
# Usage: ./run_k_sweep.sh [csv_path]       env: SEEDS, TRIALS, JOBS (see sweep_lib.sh)
set -euo pipefail
cd "$(dirname "$0")"
source sweep_lib.sh

N=20
P=0.3
KS="$(seq 1 20)"
TRIALS="${TRIALS:-2000}"
SEEDS="${SEEDS:-$(seq 0 9)}"
CSV="${1:-results/k_sweep_n${N}_p${P}.csv}"

build
for SEED in $SEEDS; do
    for K in $KS; do
        echo "$N $P $K $TRIALS $SEED"
    done
done | run_jobs "$CSV" "${CSV%.csv}_prefix.csv"

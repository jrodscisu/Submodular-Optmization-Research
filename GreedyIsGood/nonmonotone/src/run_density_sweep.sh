#!/usr/bin/env bash
# Experiment 2 -- bound quality vs. edge density.
# Sweep p = 0.1..0.9 on n=20 graphs for a few values of k (OPT by brute force).
# Usage: ./run_density_sweep.sh [csv_path]  env: SEEDS, TRIALS, JOBS (see sweep_lib.sh)
set -euo pipefail
cd "$(dirname "$0")"
source sweep_lib.sh

N=20
PS="0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9"
KS="${KS:-3 5 10}"
TRIALS="${TRIALS:-2000}"
SEEDS="${SEEDS:-$(seq 0 9)}"
CSV="${1:-results/density_sweep_n${N}.csv}"

build
for SEED in $SEEDS; do
    for P in $PS; do
        for K in $KS; do
            echo "$N $P $K $TRIALS $SEED"
        done
    done
done | run_jobs "$CSV"

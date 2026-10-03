#!/usr/bin/env bash
# Experiment 3 -- scaling to graphs where OPT is out of reach.
# Sweep n = 20..320 with p=0.3 and k in {5,10,20,40} (k <= n/2). OPT is only brute-forced
# for n <= BRUTE_MAX_N (default 20), so for larger n the dual bound is the best certificate.
# Usage: ./run_size_sweep.sh [csv_path]     env: SEEDS, TRIALS, JOBS (see sweep_lib.sh)
set -euo pipefail
cd "$(dirname "$0")"
source sweep_lib.sh

P=0.3
NS="${NS:-20 40 80 160 320}"
KS="${KS:-5 10 20 40}"
TRIALS="${TRIALS:-1000}"
SEEDS="${SEEDS:-$(seq 0 4)}"
CSV="${1:-results/size_sweep_p${P}.csv}"

build
for SEED in $SEEDS; do
    for N in $NS; do
        for K in $KS; do
            if [ $((2 * K)) -le "$N" ]; then echo "$N $P $K $TRIALS $SEED"; fi
        done
    done
done | run_jobs "$CSV"

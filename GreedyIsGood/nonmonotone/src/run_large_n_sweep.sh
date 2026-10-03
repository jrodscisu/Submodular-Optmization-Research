#!/usr/bin/env bash
# Experiment 4 -- k sweep on graphs larger than n=20 (OPT out of reach of brute force).
# For each n in NS, sweep ~20 values of k in [1, n/2] on G(n, p=0.3) over several seeds.
# Bounds are compared via the certified ratio ALG / bound and against the best solution
# found (max of plain greedy and the best random-greedy run) instead of OPT.
# Everything (CSVs, logs, figures) goes to OUT_DIR, one CSV (+ prefix CSV) per n.
# Usage: ./run_large_n_sweep.sh [out_dir]    env: NS, SEEDS, TRIALS, JOBS (see sweep_lib.sh)
set -euo pipefail
cd "$(dirname "$0")"
source sweep_lib.sh

P=0.3
NS="${NS:-40 80 160 320}"
TRIALS="${TRIALS:-1000}"
SEEDS="${SEEDS:-$(seq 0 4)}"
OUT_DIR="${1:-results_large_n}"

build
for N in $NS; do
    STEP=$(( N / 40 > 0 ? N / 40 : 1 ))    # ~20 values of k per n
    KS="$( (echo 1; seq "$STEP" "$STEP" $((N / 2))) | sort -nu)"
    CSV="$OUT_DIR/k_sweep_n${N}_p${P}.csv"
    for SEED in $SEEDS; do
        for K in $KS; do
            echo "$N $P $K $TRIALS $SEED"
        done
    done | run_jobs "$CSV" "${CSV%.csv}_prefix.csv"
done

"${PYTHON:-python3}" plot_maxcut_results.py --results "$OUT_DIR" --out "$OUT_DIR/figures" \
    || echo "Plotting failed (needs pandas + matplotlib); rerun:" \
            "python3 plot_maxcut_results.py --results $OUT_DIR --out $OUT_DIR/figures" >&2

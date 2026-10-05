#!/usr/bin/env bash
# Violations study for the original max-cut k sweep: the monotone methods (--monotone-only) on the same
# graphs as results/k_sweep_n20_p0.3_baselines.csv (generic port, seeds 0-9), then common/violations.py.
# Usage: ./run_violations.sh        env: JOBS, PYTHON (needs numpy, scipy, pandas)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 maxcut_baselines.cpp -o maxcut_baselines

for s in $(seq 0 9); do
    echo "maxcut_generic --n 20 --p 0.3 --ks 1:20 --trials 0 --seed $s"
done | VIOLATIONS=1 run_jobs ./maxcut_baselines results

"${PYTHON:-python3}" ../common/violations.py results

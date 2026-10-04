#!/usr/bin/env bash
# B2-B4 baselines for the original max-cut k sweep (results/k_sweep_n20_p0.3.csv: G(20, 0.3),
# k = 1..20, seeds 0-9). The baselines are computed with the generic code on the same graphs
# (maxcut_baselines.cpp), then merged onto the original rows -> results/k_sweep_n20_p0.3_baselines.csv
# after checking that OPT, top-k, greedy and the dual bound agree with the original run.
# Usage: ./run_baselines.sh        env: JOBS, PYTHON (needs numpy + scipy)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 maxcut_baselines.cpp -o maxcut_baselines

for s in $(seq 0 9); do
    echo "maxcut_generic --n 20 --p 0.3 --ks 1:20 --trials 0 --seed $s"
done | BASELINES=1 run_jobs ./maxcut_baselines results

"${PYTHON:-python3}" ../common/baselines_lp.py --maxcut-merge results/k_sweep_n20_p0.3.csv \
    results/maxcut_generic_baselines.csv results/k_sweep_n20_p0.3_baselines.csv

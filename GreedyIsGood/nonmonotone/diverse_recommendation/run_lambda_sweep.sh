#!/usr/bin/env bash
# lambda sweep (controlled non-monotonicity) for the violations study: diverse recommendation on the
# n = 20 MovieLens instances (20 movies sampled from the 200 most rated, seeds 0-9 as for ml20_*),
# k = 1..20, lambda in {0, 0.1, 0.25, 0.5, 0.75, 1.0, 1.5}; OPT by brute force for every row.
# f is monotone for lambda <= 1/2 and non-negative for lambda <= 1 (lambda = 1.5 can be negative).
# Runs NM-Dual + all baselines (B2/B5 solved) + the monotone methods (--monotone) in
# results/lambda_sweep/ (the existing results are not touched), then common/violations.py, which
# writes results/lambda_sweep_violations.csv and requires 0 violations of every method at lambda = 0.
# Usage: ./run_lambda_sweep.sh       env: JOBS, PYTHON (needs numpy, scipy, pandas)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 diverse_rec.cpp -o diverse_rec
ML=../../../Coverage/data/ML/ml-1m/ratings.dat
RES=results/lambda_sweep

for lam in 0 0.1 0.25 0.5 0.75 1.0 1.5; do
    for s in $(seq 0 9); do
        echo "ml20_lambda$lam --movielens $ML --top 200 --sample 20 --lambda $lam --ks 1:20 --trials 1000 --seed $s"
    done
done | BASELINES=1 EXTRA_ARGS="--monotone" run_jobs ./diverse_rec "$RES"

"${PYTHON:-python3}" ../common/baselines_lp.py "$RES"
"${PYTHON:-python3}" ../common/hybrid_lp.py "$RES"
"${PYTHON:-python3}" ../common/violations.py "$RES" --lambda-out results/lambda_sweep_violations.csv

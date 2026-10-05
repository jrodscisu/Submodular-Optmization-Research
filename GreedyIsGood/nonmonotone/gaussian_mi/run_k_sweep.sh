#!/usr/bin/env bash
# k sweep for Gaussian mutual information I(X_S; X_{V\S}).
#   synthetic_gp_n20   GP on 20 random points in [0,1]^2, 10 seeds, k = 1..20 (OPT by brute force)
#   synthetic_gp_n100  GP on 100 random points, 5 seeds, k = 1..50
#   intel              Intel Berkeley lab temperature covariance (52 motes), k = 1..52,
#                      OPT by brute force for k <= 5
# Usage: ./run_k_sweep.sh [instance ...]     (default: all)    env: JOBS, PYTHON
#        BASELINES=1 ./run_k_sweep.sh  reruns the same jobs with the B2-B4 baselines
#        (results/<instance>_baselines.csv; needs numpy + scipy)
#        VIOLATIONS=1 ./run_k_sweep.sh  monotone methods on the same jobs (results/<instance>_violations.csv;
#        needs the BASELINES=1 results)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 gaussian_mi.cpp -o gaussian_mi
[ -f data/intel_temp_cov.txt ] || data/download.sh

want() { [ -z "$SELECTED" ] || [[ " ${SELECTED} " == *" $1 "* ]]; }
SELECTED="$*"
{
    if want synthetic_gp_n20; then
        for s in $(seq 0 9); do echo "synthetic_gp_n20 --synthetic 20 --ks 1:20 --trials 1000 --seed $s"; done
    fi
    if want synthetic_gp_n100; then
        for s in $(seq 0 4); do echo "synthetic_gp_n100 --synthetic 100 --ks 1:50 --trials 300 --seed $s"; done
    fi
    if want intel; then
        echo "intel --cov data/intel_temp_cov.txt --ks 1:52 --trials 1000 --brute-budget 1e7 --seed 0"
    fi
} | run_jobs ./gaussian_mi results

if [ "${VIOLATIONS:-0}" = 1 ]; then  # monotone methods vs. the _baselines results (violations study)
    "${PYTHON:-python3}" ../common/violations.py results
elif [ "${BASELINES:-0}" = 1 ]; then  # solve the B2 LPs and run the validity checks
    "${PYTHON:-python3}" ../common/baselines_lp.py results
    "${PYTHON:-python3}" ../common/hybrid_lp.py results     # B5 hybrid LP, checks, _mono columns
else
    "${PYTHON:-python3}" ../common/plot_k_sweep.py --results results --out figures \
        || echo "plotting skipped (needs pandas + matplotlib)" >&2
fi

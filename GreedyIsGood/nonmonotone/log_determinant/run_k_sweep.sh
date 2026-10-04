#!/usr/bin/env bash
# k sweep for log-determinant (DPP MAP) with an RBF kernel, L = alpha (K + 1e-3 I).
# alpha sets where f turns non-monotone (plain greedy stops after ~15 picks at alpha = 3 and
# ~32 at alpha = 10 on wine n = 100).
#   synthetic_n20_alpha3  Gaussian mixture, 10 seeds, k = 1..20 (OPT by brute force)
#   wine_n100_alpha*      UCI wine-quality-red, 100 sampled wines, 5 seeds, k = 1..50
#   wine_n300_alpha*      300 sampled wines, 2 seeds, k up to 100, dual over every 5th prefix
# Usage: ./run_k_sweep.sh [instance ...]     (default: all)    env: JOBS, PYTHON
#        BASELINES=1 ./run_k_sweep.sh  reruns the same jobs with the B2-B4 baselines
#        (results/<instance>_baselines.csv; needs numpy + scipy)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 log_det.cpp -o log_det
[ -f data/winequality-red.csv ] || data/download.sh
WINE="--features data/winequality-red.csv --drop-last"

want() { [ -z "$SELECTED" ] || [[ " ${SELECTED} " == *" $1 "* ]]; }
SELECTED="$*"
{
    if want synthetic_n20_alpha3; then
        for s in $(seq 0 9); do echo "synthetic_n20_alpha3 --synthetic 20 --alpha 3 --ks 1:20 --trials 1000 --seed $s"; done
    fi
    for a in 3 10; do
        if want "wine_n100_alpha$a"; then
            for s in $(seq 0 4); do echo "wine_n100_alpha$a $WINE --sample 100 --alpha $a --ks 1:50 --trials 300 --seed $s"; done
        fi
        if want "wine_n300_alpha$a"; then
            for s in 0 1; do
                echo "wine_n300_alpha$a $WINE --sample 300 --alpha $a --ks 1,5:100:5 --chain-stride 5 --trials 50 --seed $s"
            done
        fi
    done
} | run_jobs ./log_det results

if [ "${BASELINES:-0}" = 1 ]; then  # solve the B2 LPs and run the validity checks
    "${PYTHON:-python3}" ../common/baselines_lp.py results
else
    "${PYTHON:-python3}" ../common/plot_k_sweep.py --results results --out figures \
        || echo "plotting skipped (needs pandas + matplotlib)" >&2
fi

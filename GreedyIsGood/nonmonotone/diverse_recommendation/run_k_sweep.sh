#!/usr/bin/env bash
# k sweep for diverse recommendation on MovieLens 1M (repo copy in Coverage/data/ML/ml-1m),
# for lambda = 0.75 and lambda = 1.0 (non-monotone range is lambda in (1/2, 1]).
#   ml20_lambda*    20 movies sampled from the 200 most rated, 10 seeds, k = 1..20 (OPT by brute force)
#   ml500_lambda*   500 movies sampled from the 1000 most rated, 3 seeds, k up to 250
# Usage: ./run_k_sweep.sh [instance ...]     (default: all)    env: JOBS, PYTHON
#        BASELINES=1 ./run_k_sweep.sh  reruns the same jobs with the B2-B4 baselines
#        (results/<instance>_baselines.csv; needs numpy + scipy)
#        VIOLATIONS=1 ./run_k_sweep.sh  monotone methods on the same jobs (results/<instance>_violations.csv;
#        needs the BASELINES=1 results)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 diverse_rec.cpp -o diverse_rec
ML=../../../Coverage/data/ML/ml-1m/ratings.dat
[ -f "$ML" ] || { echo "MovieLens ratings not found at $ML" >&2; exit 1; }

want() { [ -z "$SELECTED" ] || [[ " ${SELECTED} " == *" $1 "* ]]; }
SELECTED="$*"
{
    for lam in 0.75 1.0; do
        if want "ml20_lambda$lam"; then
            for s in $(seq 0 9); do
                echo "ml20_lambda$lam --movielens $ML --top 200 --sample 20 --lambda $lam --ks 1:20 --trials 1000 --seed $s"
            done
        fi
        if want "ml500_lambda$lam"; then
            for s in 0 1 2; do
                echo "ml500_lambda$lam --movielens $ML --top 1000 --sample 500 --lambda $lam --ks 1,10:250:10 --trials 200 --seed $s"
            done
        fi
    done
} | run_jobs ./diverse_rec results

if [ "${VIOLATIONS:-0}" = 1 ]; then  # monotone methods vs. the _baselines results (violations study)
    "${PYTHON:-python3}" ../common/violations.py results
elif [ "${BASELINES:-0}" = 1 ]; then  # solve the B2 LPs and run the validity checks
    "${PYTHON:-python3}" ../common/baselines_lp.py results
    "${PYTHON:-python3}" ../common/hybrid_lp.py results     # B5 hybrid LP, checks, _mono columns
else
    "${PYTHON:-python3}" ../common/plot_k_sweep.py --results results --out figures \
        || echo "plotting skipped (needs pandas + matplotlib)" >&2
fi

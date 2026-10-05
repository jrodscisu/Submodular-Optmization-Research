#!/usr/bin/env bash
# k sweep for revenue maximization: dual bound vs. top-k / total bounds, plain and random greedy.
#   synthetic_n20   G(20, 0.3) with U(0,1) weights, 10 seeds, k = 1..20 (OPT by brute force)
#   synthetic_n200  G(200, 0.1) with U(0,1) weights, 5 seeds, k up to 100
#   facebook        SNAP ego-Facebook (n = 4039), U(0,1) weights, 3 weight seeds, k up to 200,
#                   dual over every 10th greedy prefix
#   youtube         com-YouTube community subgraph (n = 3804), 3 weight seeds, k up to 200,
#                   dual over every 5th greedy prefix
# Usage: ./run_k_sweep.sh [instance ...]     (default: all)    env: JOBS, PYTHON
#        BASELINES=1 ./run_k_sweep.sh  reruns the same jobs with the B2-B4 baselines
#        (results/<instance>_baselines.csv; needs numpy + scipy)
#        VIOLATIONS=1 ./run_k_sweep.sh  monotone methods on the same jobs (results/<instance>_violations.csv;
#        needs the BASELINES=1 results)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 revenue_max.cpp -o revenue_max
[ -f data/youtube_cmty_3000.txt ] || data/download.sh

want() { [ -z "$SELECTED" ] || [[ " ${SELECTED} " == *" $1 "* ]]; }
SELECTED="$*"
{
    if want synthetic_n20; then
        for s in $(seq 0 9); do echo "synthetic_n20 --synthetic 20 --p 0.3 --ks 1:20 --trials 1000 --seed $s"; done
    fi
    if want synthetic_n200; then
        for s in $(seq 0 4); do echo "synthetic_n200 --synthetic 200 --p 0.1 --ks 1,5:100:5 --trials 300 --seed $s"; done
    fi
    if want facebook; then
        for s in 0 1 2; do
            echo "facebook --graph data/facebook_combined.txt --ks 1,10:200:10 --chain-stride 10 --trials 50 --seed $s"
        done
    fi
    if want youtube; then
        for s in 0 1 2; do
            echo "youtube --graph data/youtube_cmty_3000.txt --ks 1,10:200:10 --chain-stride 5 --trials 100 --seed $s"
        done
    fi
} | run_jobs ./revenue_max results

if [ "${VIOLATIONS:-0}" = 1 ]; then  # monotone methods vs. the _baselines results (violations study)
    "${PYTHON:-python3}" ../common/violations.py results
elif [ "${BASELINES:-0}" = 1 ]; then  # solve the B2 LPs and run the validity checks
    "${PYTHON:-python3}" ../common/baselines_lp.py results
    "${PYTHON:-python3}" ../common/hybrid_lp.py results     # B5 hybrid LP, checks, _mono columns
else
    "${PYTHON:-python3}" ../common/plot_k_sweep.py --results results --out figures \
        || echo "plotting skipped (needs pandas + matplotlib)" >&2
fi

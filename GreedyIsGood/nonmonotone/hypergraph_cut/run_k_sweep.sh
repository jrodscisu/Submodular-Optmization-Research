#!/usr/bin/env bash
# k sweep for hypergraph cut: dual bound vs. top-k / total-weight bounds, plain and random greedy.
#   synthetic_n20    20 nodes, 60 random hyperedges of size 2..5, 10 seeds, k = 1..20 (OPT by brute force)
#   synthetic_n500   500 nodes, 2000 hyperedges, 5 seeds, k up to 250
#   mag10_top1000    co-authorship hypergraph (cat-edge-MAG-10, 1000 most prolific authors), k up to 300
# Usage: ./run_k_sweep.sh [instance ...]     (default: all)    env: JOBS, PYTHON
#        BASELINES=1 ./run_k_sweep.sh  reruns the same jobs with the B2-B4 baselines
#        (results/<instance>_baselines.csv; needs numpy + scipy)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 hypergraph_cut.cpp -o hypergraph_cut
[ -f data/mag10_top1000.txt ] || data/download.sh

want() { [ -z "$SELECTED" ] || [[ " ${SELECTED} " == *" $1 "* ]]; }
SELECTED="$*"
{
    if want synthetic_n20; then
        for s in $(seq 0 9); do echo "synthetic_n20 --synthetic 20 --edges 60 --ks 1:20 --trials 1000 --seed $s"; done
    fi
    if want synthetic_n500; then
        for s in $(seq 0 4); do echo "synthetic_n500 --synthetic 500 --edges 2000 --ks 1,10:250:10 --trials 200 --seed $s"; done
    fi
    if want mag10_top1000; then
        echo "mag10_top1000 --hypergraph data/mag10_top1000.txt --ks 1,10:300:10 --trials 200 --seed 0"
    fi
} | run_jobs ./hypergraph_cut results

if [ "${BASELINES:-0}" = 1 ]; then  # solve the B2 LPs and run the validity checks
    "${PYTHON:-python3}" ../common/baselines_lp.py results
else
    "${PYTHON:-python3}" ../common/plot_k_sweep.py --results results --out figures \
        || echo "plotting skipped (needs pandas + matplotlib)" >&2
fi

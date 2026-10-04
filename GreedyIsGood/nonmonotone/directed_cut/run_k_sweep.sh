#!/usr/bin/env bash
# k sweep for directed cut: dual bound vs. top-k / total-weight bounds, plain and random greedy.
#   synthetic_n20   G(20, 0.3) random orientations, 10 seeds, k = 1..20 (OPT by brute force)
#   synthetic_n200  G(200, 0.1), 5 seeds, k up to 100
#   email_eu_core   SNAP email-Eu-core (n = 1005), k up to 500
#   wiki_vote       SNAP wiki-Vote (n = 7115), k up to 200, dual over every 10th greedy prefix
# Usage: ./run_k_sweep.sh [instance ...]     (default: all)    env: JOBS, PYTHON
#        BASELINES=1 ./run_k_sweep.sh  reruns the same jobs with the B2-B4 baselines
#        (results/<instance>_baselines.csv; needs numpy + scipy)
set -euo pipefail
cd "$(dirname "$0")"
source ../common/run_jobs.sh
g++ -O2 -std=c++17 directed_cut.cpp -o directed_cut
[ -f data/wiki-Vote.txt ] || data/download.sh

want() { [ -z "$SELECTED" ] || [[ " ${SELECTED} " == *" $1 "* ]]; }
SELECTED="$*"
{
    if want synthetic_n20; then
        for s in $(seq 0 9); do echo "synthetic_n20 --synthetic 20 --p 0.3 --ks 1:20 --trials 1000 --seed $s"; done
    fi
    if want synthetic_n200; then
        for s in $(seq 0 4); do echo "synthetic_n200 --synthetic 200 --p 0.1 --ks 1,5:100:5 --trials 300 --seed $s"; done
    fi
    if want email_eu_core; then
        echo "email_eu_core --graph data/email-Eu-core.txt --ks 1,20:500:20 --trials 200 --seed 0"
    fi
    if want wiki_vote; then
        echo "wiki_vote --graph data/wiki-Vote.txt --ks 1,10:200:10 --chain-stride 10 --trials 50 --seed 0"
    fi
} | run_jobs ./directed_cut results

if [ "${BASELINES:-0}" = 1 ]; then  # solve the B2 LPs and run the validity checks
    "${PYTHON:-python3}" ../common/baselines_lp.py results
else
    "${PYTHON:-python3}" ../common/plot_k_sweep.py --results results --out figures \
        || echo "plotting skipped (needs pandas + matplotlib)" >&2
fi

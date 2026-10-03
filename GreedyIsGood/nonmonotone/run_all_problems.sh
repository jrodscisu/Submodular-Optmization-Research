#!/usr/bin/env bash
# Run the k sweep of every problem (each writes <problem>/results and <problem>/figures), then
# the cross-problem overview figure (figures/) and summary table (RESULTS_k_sweep.md).
# Usage: ./run_all_problems.sh [problem ...]     env: JOBS, PYTHON
#   problems: directed_cut revenue_max diverse_recommendation log_determinant gaussian_mi hypergraph_cut
set -euo pipefail
cd "$(dirname "$0")"
PROBLEMS="${*:-gaussian_mi hypergraph_cut directed_cut diverse_recommendation log_determinant revenue_max}"

for p in $PROBLEMS; do
    echo "=================== $p"
    "$p/run_k_sweep.sh"
done

ALL="directed_cut revenue_max diverse_recommendation log_determinant gaussian_mi hypergraph_cut"
RES=""
for p in $ALL; do [ -d "$p/results" ] && RES="$RES $p/results"; done
# shellcheck disable=SC2086
"${PYTHON:-python3}" common/plot_k_sweep.py --results $RES --out figures --summary RESULTS_k_sweep.md \
    || echo "summary skipped (needs pandas + matplotlib)" >&2

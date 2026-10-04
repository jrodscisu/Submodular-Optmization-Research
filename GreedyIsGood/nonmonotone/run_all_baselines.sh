#!/usr/bin/env bash
# Rerun every problem's k-sweep grid with the B2-B4 baselines (same instances and seeds), solve the
# B2 LPs, run the validity checks, then write the figures and baselines_report.md.
# Usage: ./run_all_baselines.sh [problem ...]     env: JOBS, PYTHON (needs numpy, scipy)
#   problems: max_cut gaussian_mi hypergraph_cut directed_cut diverse_recommendation log_determinant revenue_max
set -euo pipefail
cd "$(dirname "$0")"
PROBLEMS="${*:-max_cut gaussian_mi hypergraph_cut directed_cut diverse_recommendation log_determinant revenue_max}"

for p in $PROBLEMS; do
    echo "=================== $p"
    if [ "$p" = max_cut ]; then src/run_baselines.sh; else BASELINES=1 "$p/run_k_sweep.sh"; fi
done

"${PYTHON:-python3}" latex/make_pgfplots.py --baselines
"${PYTHON:-python3}" common/baselines_report.py --out baselines_report_tables.md

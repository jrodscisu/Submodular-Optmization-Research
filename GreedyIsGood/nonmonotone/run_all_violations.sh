#!/usr/bin/env bash
# Violations study: run the published monotone methods (M1 BQS Dual, M2 Marginal without penalty,
# M3 Zhang-Tang-Tang Lambda^{3*}) and the ablations A1/A2 on every existing instance (same instances,
# seeds and base sets as the *_baselines.csv results, which must exist: ./run_all_baselines.sh),
# then the diverse-rec lambda sweep, the figures and violations_report_tables.md.
# Usage: ./run_all_violations.sh [problem ...]     env: JOBS, PYTHON (needs numpy, scipy, pandas)
set -euo pipefail
cd "$(dirname "$0")"
PROBLEMS="${*:-max_cut gaussian_mi hypergraph_cut directed_cut diverse_recommendation log_determinant revenue_max lambda_sweep}"

for p in $PROBLEMS; do
    echo "=================== $p"
    case "$p" in
        max_cut) src/run_violations.sh ;;
        lambda_sweep) diverse_recommendation/run_lambda_sweep.sh ;;
        *) VIOLATIONS=1 "$p/run_k_sweep.sh" ;;
    esac
done

"${PYTHON:-python3}" latex/make_violation_plots.py
"${PYTHON:-python3}" common/violations_report.py --out violations_report_tables.md

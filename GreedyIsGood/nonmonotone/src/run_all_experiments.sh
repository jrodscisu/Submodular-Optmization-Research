#!/usr/bin/env bash
# Run every experiment and produce the plots in figures/.
# Usage: ./run_all_experiments.sh           env: SEEDS, TRIALS, JOBS, PYTHON
set -euo pipefail
cd "$(dirname "$0")"

./run_k_sweep.sh
./run_density_sweep.sh
./run_size_sweep.sh
"${PYTHON:-python3}" plot_maxcut_results.py

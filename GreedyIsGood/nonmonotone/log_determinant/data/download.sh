#!/usr/bin/env bash
# UCI Wine Quality (red): 1,599 wines x 11 physico-chemical features (+ quality label, dropped).
set -euo pipefail
cd "$(dirname "$0")"
if [ ! -f winequality-red.csv ]; then
    curl -fsSLO https://archive.ics.uci.edu/ml/machine-learning-databases/wine-quality/winequality-red.csv
    echo "downloaded winequality-red.csv"
fi

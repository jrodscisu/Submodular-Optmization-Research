#!/usr/bin/env bash
# Intel Berkeley Research Lab sensor data (2.3M readings from 54 motes, Feb 28 - Apr 5 2004),
# reduced by prepare_intel.py to the covariance of the motes' temperature time series.
set -euo pipefail
cd "$(dirname "$0")"
if [ ! -f data.txt.gz ]; then
    curl -fsSLO https://db.csail.mit.edu/labdata/data.txt.gz
    echo "downloaded data.txt.gz"
fi
python3 prepare_intel.py

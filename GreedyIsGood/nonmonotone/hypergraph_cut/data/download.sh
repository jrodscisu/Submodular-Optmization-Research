#!/usr/bin/env bash
# Co-authorship hypergraph cat-edge-MAG-10 (Amburg, Veldt, Benson 2020): 80,198 authors,
# 51,889 papers as hyperedges. prepare_mag.py keeps the most prolific authors
# (MAG_AUTHORS, default 1000).
set -euo pipefail
cd "$(dirname "$0")"
if [ ! -f cat-edge-MAG-10/hyperedges.txt ]; then
    curl -fsSL -o cat-edge-MAG-10.zip \
        "https://drive.usercontent.google.com/download?id=1qE0SRcFtLYtNtMS6VB7CjirV8rugTcrT&export=download&confirm=t"
    unzip -oq cat-edge-MAG-10.zip
    rm cat-edge-MAG-10.zip
    echo "downloaded cat-edge-MAG-10"
fi
python3 prepare_mag.py --authors "${MAG_AUTHORS:-1000}"

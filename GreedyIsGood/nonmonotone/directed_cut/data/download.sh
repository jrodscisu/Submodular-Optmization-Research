#!/usr/bin/env bash
# SNAP directed graphs: email-Eu-core (1,005 nodes, 25,571 edges) and wiki-Vote (7,115 / 103,689).
set -euo pipefail
cd "$(dirname "$0")"
for g in email-Eu-core wiki-Vote; do
    if [ ! -f "$g.txt" ]; then
        curl -fsSL "https://snap.stanford.edu/data/$g.txt.gz" | gunzip > "$g.txt"
        echo "downloaded $g.txt"
    fi
done

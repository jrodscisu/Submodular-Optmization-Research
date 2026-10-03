#!/usr/bin/env bash
# SNAP ego-Facebook (4,039 nodes, 88,234 edges) and a community-based subgraph of com-YouTube
# (see prepare_youtube.py; YOUTUBE_NODES sets its size, default 3000).
set -euo pipefail
cd "$(dirname "$0")"
if [ ! -f facebook_combined.txt ]; then
    curl -fsSL https://snap.stanford.edu/data/facebook_combined.txt.gz | gunzip > facebook_combined.txt
    echo "downloaded facebook_combined.txt"
fi
B=https://snap.stanford.edu/data/bigdata/communities
[ -f com-youtube.ungraph.txt.gz ] || curl -fsSLO "$B/com-youtube.ungraph.txt.gz"
[ -f com-youtube.top5000.cmty.txt.gz ] || curl -fsSLO "$B/com-youtube.top5000.cmty.txt.gz"
python3 prepare_youtube.py --nodes "${YOUTUBE_NODES:-3000}"

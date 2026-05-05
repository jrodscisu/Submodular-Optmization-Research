#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_FILE="$SCRIPT_DIR/all_bygenre_best_gaps.txt"
PYTHON_SCRIPT="$SCRIPT_DIR/compute_best_gap.py"

: > "$OUTPUT_FILE"

for file in "$SCRIPT_DIR"/bygenre_output/*/*.txt; do
  if [[ -f "$file" ]]; then
    python3 "$PYTHON_SCRIPT" "$file" "$OUTPUT_FILE"
  fi
done

echo "Wrote summary to $OUTPUT_FILE"

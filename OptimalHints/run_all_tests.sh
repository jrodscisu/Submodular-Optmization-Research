#!/bin/bash
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
EXE="$DIR/greedy"   # change to "./mtx2mins" if your executable is named mtx2mins
DATA_DIR="$DIR/../LibMVC/data/temp_data"

if [ ! -x "$EXE" ]; then
  echo "Error: executable $EXE not found or not executable."
  echo "Build it first, e.g.:"
  echo "  g++ -std=c++17 -O2 -o \"$DIR/greedy\" main.cpp"
  exit 1
fi

shopt -s nullglob
for file in "$DATA_DIR"/*.mis; do
  echo "Running: $EXE \"$file\""
  "$EXE" "$file"
done

echo "All files processed."
#!/usr/bin/env bash
# Build every problem binary (and the max-cut port check).
# Usage: common/build_all.sh        env: CXXFLAGS (default -O2 -std=c++17)
set -euo pipefail
cd "$(dirname "$0")/.."
FLAGS="${CXXFLAGS:--O2 -std=c++17}"

g++ $FLAGS directed_cut/directed_cut.cpp -o directed_cut/directed_cut
g++ $FLAGS revenue_max/revenue_max.cpp -o revenue_max/revenue_max
g++ $FLAGS diverse_recommendation/diverse_rec.cpp -o diverse_recommendation/diverse_rec
g++ $FLAGS log_determinant/log_det.cpp -o log_determinant/log_det
g++ $FLAGS gaussian_mi/gaussian_mi.cpp -o gaussian_mi/gaussian_mi
g++ $FLAGS hypergraph_cut/hypergraph_cut.cpp -o hypergraph_cut/hypergraph_cut
g++ $FLAGS common/port_check.cpp -o common/port_check
echo "built all binaries"

#!/usr/bin/env bash
# Correctness checks for every problem on small instances (n <= 20, OPT by brute force):
#   --check-oracle : incremental gain(v) == f(S + v) - f(S - v) on random sets
#   --check-direct : the cached k-sweep dual == dual_wrapper(k, plain_greedy(k)) for every k
#   dual_valid     : dual bound >= OPT (and >= every solution found) for every k
# plus the max-cut port check against src/results/k_sweep_n20_p0.3.csv.
# Usage: common/validate.sh            (run from anywhere; builds first)
set -euo pipefail
cd "$(dirname "$0")/.."
common/build_all.sh

OUT="$(mktemp -d)"
ML="../../Coverage/data/ML/ml-1m/ratings.dat"
FAILED=0

check() {  # check NAME CSV
    local name="$1" csv="$2"
    # columns: opt=9, dual_bound=12, dual_valid=21
    if awk -F, 'NR > 1 { rows++; if ($21 != 1 || ($9 != "nan" && $12 + 1e-7 * ($12 > 1 ? $12 : 1) < $9)) bad++ }
                END { printf "  %-28s %3d rows, %d invalid\n", FILENAME, rows, bad; exit bad > 0 }' "$csv"; then
        echo "  ok    $name"
    else
        echo "  FAIL  $name"; FAILED=1
    fi
}

run() {  # run NAME BINARY ARGS...
    local name="$1" bin="$2"; shift 2
    if ! "$bin" "$@" --check-oracle --check-direct --trials 200 --out "$OUT/$name.csv" > "$OUT/$name.log" 2>&1; then
        echo "  FAIL  $name (see $OUT/$name.log)"; tail -3 "$OUT/$name.log"; FAILED=1; return
    fi
    grep -h "oracle check" "$OUT/$name.log" | sed "s/^ */  $name: /"
    check "$name" "$OUT/$name.csv"
}

echo "== max-cut port check"
common/port_check src/results/k_sweep_n20_p0.3.csv | tail -1

for seed in 0 1 2; do
    echo "== seed $seed"
    run "directed_cut_s$seed" directed_cut/directed_cut --synthetic 18 --p 0.3 --ks 1:18 --seed $seed
    run "revenue_max_s$seed" revenue_max/revenue_max --synthetic 18 --p 0.3 --ks 1:18 --seed $seed
    run "diverse_rec_l075_s$seed" diverse_recommendation/diverse_rec --movielens "$ML" --top 200 --sample 18 \
        --lambda 0.75 --ks 1:18 --seed $seed
    run "diverse_rec_l1_s$seed" diverse_recommendation/diverse_rec --movielens "$ML" --top 200 --sample 18 \
        --lambda 1.0 --ks 1:18 --seed $seed
    run "log_det_s$seed" log_determinant/log_det --synthetic 16 --alpha 3 --ks 1:16 --seed $seed
    run "gaussian_mi_s$seed" gaussian_mi/gaussian_mi --synthetic 16 --ks 1:16 --seed $seed
    run "hypergraph_cut_s$seed" hypergraph_cut/hypergraph_cut --synthetic 18 --edges 40 --ks 1:18 --seed $seed
done

echo "outputs in $OUT"
[ $FAILED = 0 ] && echo "ALL CHECKS PASSED" || { echo "SOME CHECKS FAILED"; exit 1; }

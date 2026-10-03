# Shared helpers for the run_*_sweep.sh experiment runners. Source it, don't execute it.
#
# Env overrides understood by every runner:
#   JOBS         parallel processes        (default: number of CPUs)
#   TRIALS       random-greedy runs / row  (default set by each runner)
#   SEEDS        graph seeds, e.g. "0 1 2" (default set by each runner)
#   BRUTE_MAX_N  brute-force OPT only when n <= this (default 20)

JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || nproc)}"
BRUTE_MAX_N="${BRUTE_MAX_N:-20}"

build() {
    g++ -O2 -std=c++17 maxcut_random_greedy.cpp -o maxcut
}

# run_jobs CSV [PREFIX_CSV] < job_lines
# Each job line on stdin is "n p k trials seed". Jobs run in parallel, each into its own
# temp CSV; they are then merged in input order into CSV (and PREFIX_CSV if given).
# The program's stdout of every job is collected in logs/<csv name>.log.
run_jobs() {
    local csv="$1" prefix_csv="${2:-}"
    local tmp; tmp="$(mktemp -d)"
    local log="logs/$(basename "${csv%.csv}").log"
    mkdir -p "$(dirname "$csv")" logs

    local njobs
    njobs=$(awk 'NF' | tee "$tmp/jobs" | wc -l | tr -d ' ')
    echo "Running $njobs jobs with $JOBS parallel processes -> $csv"

    # xargs appends each line's fields ("id n p k trials seed") after the fixed args
    local status=0
    awk '{ print NR, $0 }' "$tmp/jobs" | xargs -P "$JOBS" -L1 sh -c '
        tmp=$1 brute=$2 want_prefix=$3 id=$4; shift 4
        if [ "$want_prefix" = 1 ]; then pfx="$tmp/p_$id.csv"; else pfx=""; fi
        if ./maxcut "$1" "$2" "$3" "$4" "$5" "$tmp/r_$id.csv" "$brute" $pfx > "$tmp/log_$id.txt" 2>&1; then
            echo "  done   n=$1 p=$2 k=$3 seed=$5"
        else
            echo "  FAILED n=$1 p=$2 k=$3 seed=$5 (see log)" >&2; exit 1
        fi
    ' _ "$tmp" "$BRUTE_MAX_N" "$([ -n "$prefix_csv" ] && echo 1 || echo 0)" || status=$?

    merge_csvs "$tmp" r "$njobs" "$csv"
    [ -n "$prefix_csv" ] && merge_csvs "$tmp" p "$njobs" "$prefix_csv"
    : > "$log"
    for i in $(seq 1 "$njobs"); do cat "$tmp/log_$i.txt" >> "$log" 2>/dev/null || true; done
    rm -rf "$tmp"
    echo "Results written to $(pwd)/$csv"
    [ $status = 0 ] || echo "WARNING: some jobs failed (exit $status); see $log" >&2
    return 0
}

# merge_csvs DIR PREFIX COUNT OUT: concatenate DIR/PREFIX_{1..COUNT}.csv keeping one header
merge_csvs() {
    local dir="$1" pre="$2" count="$3" out="$4" first=1 missing=0
    : > "$out"
    for i in $(seq 1 "$count"); do
        local f="$dir/${pre}_$i.csv"
        if [ ! -f "$f" ]; then missing=$((missing + 1)); continue; fi
        if [ $first = 1 ]; then cat "$f" >> "$out"; first=0; else tail -n +2 "$f" >> "$out"; fi
    done
    [ $missing -gt 0 ] && echo "WARNING: $missing jobs produced no output for $out" >&2
    return 0
}

# Parallel job runner for the per-problem run_k_sweep.sh scripts. Source it, don't execute it.
#
#   run_jobs BINARY RESULTS_DIR < job_lines
#
# Each job line is "LABEL ARG ARG ..." (whitespace separated, no quoting). Every job runs
#   BINARY ARGS --out <tmp>.csv --prefix-out <tmp>_prefix.csv
# in parallel; afterwards all jobs with the same LABEL are merged, in input order, into
# RESULTS_DIR/LABEL.csv and RESULTS_DIR/LABEL_prefix.csv, and their stdout into
# RESULTS_DIR/logs/LABEL.log. Existing files for a LABEL are replaced.
#
# Env: JOBS (parallel processes, default = #CPUs)

JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || nproc)}"

run_jobs() {
    local bin="$1" res="$2"
    local tmp; tmp="$(mktemp -d)"
    mkdir -p "$res/logs"
    awk 'NF { print NR, $0 }' > "$tmp/jobs"
    echo "Running $(wc -l < "$tmp/jobs" | tr -d ' ') jobs of $bin with $JOBS parallel processes"

    local status=0
    xargs -P "$JOBS" -L1 sh -c '
        tmp=$1 bin=$2 id=$3 label=$4; shift 4
        start=$(date +%s)
        if "$bin" "$@" --out "$tmp/${label}__$id.csv" --prefix-out "$tmp/${label}__${id}_prefix.csv" \
                > "$tmp/${label}__$id.log" 2>&1; then
            echo "  done   $label  $* ($(( $(date +%s) - start ))s)"
        else
            echo "  FAILED $label  $* (log: $tmp/${label}__$id.log)" >&2; exit 1
        fi
    ' _ "$tmp" "$bin" < "$tmp/jobs" || status=$?

    local label
    for label in $(awk '{ print $2 }' "$tmp/jobs" | awk '!seen[$0]++'); do
        local out="$res/$label.csv" pout="$res/${label}_prefix.csv" log="$res/logs/$label.log" first=1
        : > "$out"; : > "$pout"; : > "$log"
        local id
        for id in $(awk -v l="$label" '$2 == l { print $1 }' "$tmp/jobs"); do
            local f="$tmp/${label}__$id"
            cat "$f.log" >> "$log" 2>/dev/null || true
            [ -f "$f.csv" ] || continue
            if [ $first = 1 ]; then
                cat "$f.csv" >> "$out"; cat "${f}_prefix.csv" >> "$pout"
                first=0
            else
                tail -n +2 "$f.csv" >> "$out"; tail -n +2 "${f}_prefix.csv" >> "$pout"
            fi
        done
        echo "  -> $out ($(($(wc -l < "$out") - 1)) rows)"
    done
    if [ $status = 0 ]; then rm -rf "$tmp"; else echo "WARNING: some jobs failed; kept $tmp" >&2; fi
    return 0
}

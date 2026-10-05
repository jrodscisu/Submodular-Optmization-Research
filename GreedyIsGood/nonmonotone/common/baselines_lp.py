"""Solve the B2 LPs, merge them into the *_baselines.csv results, and run the validity checks.

For each <results>/<label>_baselines.csv (written by a problem binary with --baselines):
  * B2: for every row (instance, seed, k), read <results>/lp_export/<instance>__seed<seed>.lp.txt
    and solve   max z  s.t.  z - coef_S . x <= const_S  for the base sets S that NM-Dual uses at k,
                sum x <= k,  0 <= x <= 1      with scipy.optimize.linprog(method="highs");
    fill lp_bound and time_lp_ms.
  * checks (mandatory), on every row with OPT: each of lp_bound, gamma1_bound, gamma1_noprune,
    mu2_bound, mu3_bound, marginal_bound >= OPT_k - 1e-6 max(1, |OPT_k|); on every row the proven
    orderings dual <= marginal <= top-k and lp <= marginal (relative tolerance 1e-6); lattice sizes
    |A*| <= |B*|; and where the unconstrained optimum was enumerated, the lattice [A*, B*] contains
    a set attaining it.
    Any failure stops the script with the problem, instance, seed, k and all values.

  --maxcut-merge ORIGINAL.csv GENERIC.csv OUT.csv merges the max-cut baselines computed with the
  generic code onto the rows of the original max-cut CSV after checking that both runs agree on
  OPT, top-k, greedy and the dual bound (i.e. are the same instances).

Usage:
  python baselines_lp.py <results_dir> [<results_dir> ...]
  python baselines_lp.py --maxcut-merge src/results/k_sweep_n20_p0.3.csv generic.csv out.csv
Requires numpy, scipy.
"""
import argparse
import csv
import glob
import math
import os
import sys
import time

import numpy as np
from scipy.optimize import linprog

NEW_BOUNDS = ["lp_bound", "gamma1_bound", "gamma1_noprune", "mu2_bound", "mu3_bound", "marginal_bound"]
# proven orderings (Marginal = NM-Dual without the caps; its S = {} term is top-k; LP <= Marginal)
ORDERINGS = [("dual_bound", "marginal_bound"), ("marginal_bound", "top_k_bound"), ("lp_bound", "marginal_bound")]


def num(x):
    try:
        return float(x)
    except (TypeError, ValueError):
        return math.nan


def read_lp(path):
    rows, uses, n = {}, {}, None
    with open(path) as f:
        for line in f:
            if line.startswith("#"):
                continue
            p = line.split()
            if p[0] == "n":
                n = int(p[1])
            elif p[0] == "P":
                rows[int(p[1])] = (float(p[2]), np.array(p[3:], dtype=float))
            elif p[0] == "K":
                uses[int(p[1])] = [int(i) for i in p[2:]]
    for i, (c, coef) in rows.items():
        if len(coef) != n or not np.all(np.isfinite(coef)) or not math.isfinite(c):
            raise SystemExit(f"bad LP row {i} in {path}")
    return n, rows, uses


def solve_lp(n, rows, prefixes, k):
    # variables [z, x_0..x_{n-1}];  minimize -z
    A = np.zeros((len(prefixes) + 1, n + 1))
    b = np.zeros(len(prefixes) + 1)
    for r, i in enumerate(prefixes):
        cst, coef = rows[i]
        A[r, 0], A[r, 1:], b[r] = 1.0, -coef, cst
    A[-1, 1:], b[-1] = 1.0, k
    c = np.zeros(n + 1)
    c[0] = -1.0
    res = linprog(c, A_ub=A, b_ub=b, bounds=[(None, None)] + [(0, 1)] * n, method="highs")
    if res.status != 0:
        raise SystemExit(f"LP failed (status {res.status}: {res.message})")
    return -res.fun


def fill_lp(path, lp_dir):
    with open(path) as f:
        recs = list(csv.DictReader(f))
        fields = list(recs[0].keys()) if recs else []
    cache = {}
    for r in recs:
        key = (r["instance"], r["seed"])
        if key not in cache:
            cache[key] = read_lp(os.path.join(lp_dir, f"{r['instance']}__seed{r['seed']}.lp.txt"))
        n, rows, uses = cache[key]
        t0 = time.perf_counter()
        r["lp_bound"] = f"{solve_lp(n, rows, uses[int(r['k'])], int(r['k'])):.10g}"
        r["time_lp_ms"] = f"{1000 * (time.perf_counter() - t0):.3f}"
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(recs)
    return recs


def check(recs, where):
    """mandatory validity checks; returns #rows with OPT checked. Exits on any violation."""
    checked, failures, bad_rows = 0, [], []
    for r in recs:
        opt = num(r["opt"])
        tag = f"{where} instance={r.get('instance', '')} seed={r['seed']} k={r['k']}"
        n_before = len(failures)
        if int(r["lattice_A_size"]) > int(r["lattice_B_size"]):
            failures.append(f"{tag}: |A*|={r['lattice_A_size']} > |B*|={r['lattice_B_size']}")
        ou, lu = num(r["opt_unconstrained"]), num(r["lattice_opt_unconstrained"])
        if not math.isnan(ou) and not (lu >= ou - 1e-6 * max(1, abs(ou))):
            failures.append(f"{tag}: lattice optimum {lu} < unconstrained optimum {ou}")
        for lo, hi in ORDERINGS:
            a, b = num(r[lo]), num(r[hi])
            if not (a <= b + 1e-6 * max(1.0, abs(b))):
                failures.append(f"{tag}: ordering {lo}={a} > {hi}={b}")
        if math.isnan(opt):
            if len(failures) > n_before:
                bad_rows.append(r)
            continue
        checked += 1
        tol = 1e-6 * max(1.0, abs(opt))
        for b in NEW_BOUNDS:
            v = num(r[b])
            if not (v >= opt - tol):
                failures.append(f"{tag}: {b}={v} < OPT={opt}")
        if len(failures) > n_before:
            bad_rows.append(r)
    if failures:
        print("VALIDITY CHECK FAILED:")
        for f in failures:
            print("  " + f)
        for r in bad_rows:
            print("  row: " + ", ".join(f"{k}={v}" for k, v in r.items()))
        raise SystemExit(1)
    return checked


def maxcut_merge(orig, generic, out):
    with open(orig) as f:
        o = list(csv.DictReader(f))
        ofields = list(o[0].keys())
    with open(generic) as f:
        g = {(r["seed"], r["k"]): r for r in csv.DictReader(f)}
    extra = [c for c in next(iter(g.values())).keys() if c not in ("problem", "instance", "chain_stride", "chain_len")
             and c not in ofields and c not in ("total_bound", "dual_bound_S0", "dual_best_prefix", "best_found",
                                                 "dual_valid", "time_dual_ms", "time_rg_ms", "time_opt_ms",
                                                 "rg_mean", "rg_std", "rg_min", "rg_max", "trials")]
    merged = []
    for r in o:
        gr = g[(r["seed"], r["k"])]
        for col in ("opt", "top_k_bound", "greedy", "dual_bound"):
            a, b = num(r[col]), num(gr[col])
            if abs(a - b) > 1e-3 * max(1.0, abs(a)):
                raise SystemExit(f"max-cut instance mismatch seed={r['seed']} k={r['k']} {col}: {a} vs {b}")
        m = dict(r)
        m["instance"] = gr["instance"]
        for c in extra:
            m[c] = gr[c]
        merged.append(m)
    with open(out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=ofields + ["instance"] + extra)
        w.writeheader()
        w.writerows({k: v for k, v in m.items() if k in ofields + ["instance"] + extra} for m in merged)
    print(f"merged {len(merged)} max-cut rows -> {out} (OPT/top-k/greedy/dual agree with the original run)")
    return merged


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("results", nargs="*")
    ap.add_argument("--maxcut-merge", nargs=3, metavar=("ORIGINAL", "GENERIC", "OUT"))
    args = ap.parse_args()

    if args.maxcut_merge:
        orig, generic, out = args.maxcut_merge
        fill_lp(generic, os.path.join(os.path.dirname(generic), "lp_export"))
        recs = maxcut_merge(orig, generic, out)
        print(f"  max_cut: {check(recs, 'max_cut')} rows with OPT checked, 0 violations")
        return

    for res in args.results:
        for path in sorted(glob.glob(os.path.join(res, "*_baselines.csv"))):
            t0 = time.time()
            recs = fill_lp(path, os.path.join(res, "lp_export"))
            n_opt = check(recs, path)
            print(f"  {path}: {len(recs)} rows, LPs solved in {time.time() - t0:.1f}s, "
                  f"{n_opt} rows with OPT checked, 0 violations")


if __name__ == "__main__":
    sys.exit(main())

"""B5 hybrid LP (B2 + NM-Dual's prefix caps), its checks, and the Phase-2 monotone columns.

For each <results>/<label>_baselines.csv (after common/baselines_lp.py) and each row
(instance, seed, k), read <results>/lp_export/<instance>__seed<seed>.hybrid.txt (written by the
problem binary with --baselines --lp-export) and solve, over the base sets S NM-Dual uses at k:

  max z  s.t.  sum_a x_a <= k                                                    (C0)
               P^S_i - P^S_{i-1} <= f(a_i | S) x_{a_i},  P^S_0 = 0,  i = 1..m    (C1)
               P^S_i <= U^S_i            (as a variable upper bound)            (C2)
               z <= f(S) + P^S_m - sum_{a in S} f(a | V-a) (1 - x_a)             (C3)
               x in [0,1]^V, z and P free (below)

with a_1..a_m = V \\ S in NM-Dual's order and U = NM-Dual's caps (U_tilde of high_cap_U).
Solved with scipy.optimize.linprog(method="highs"); any non-optimal status stops the script.

Adds the columns hybrid_bound, best_bound = min(dual_bound, hybrid_bound), hybrid_status,
hybrid_rows, hybrid_cols, time_hybrid_solve_ms (time_hybrid_export_ms comes from the binary),
hybrid_certificate, and (Phase 2) dual_bound_mono, lp_bound_mono, hybrid_bound_mono,
best_bound_mono = min over k' >= k of the same bound (same instance and seed).

Mandatory checks (any failure stops with all values):
  1. hybrid_bound >= OPT_k - 1e-6 max(1, |OPT_k|) on rows with OPT (and the same for every _mono column)
  2. hybrid_bound <= lp_bound + 1e-6 max(1, |lp_bound|) on every row
  3. certificate: where an optimal set O was exported, the point x = 1_O, z = f(O),
     P^S_i = f_S(O \\ S among the first i positions) satisfies C0-C3 (relative tolerance 1e-6)

Usage: python hybrid_lp.py <results_dir> [...]   env JOBS (parallel LP solves)   needs numpy, scipy
"""
import csv
import glob
import math
import multiprocessing as mp
import os
import sys
import time

import numpy as np
import scipy.sparse as sp
from scipy.optimize import linprog

TOL = 1e-6
DATA = {}  # (instance, seed) -> parsed hybrid file; filled in the parent, inherited by forked workers


def num(x):
    try:
        return float(x)
    except (TypeError, ValueError):
        return math.nan


def read_hybrid(path):
    d = {"sets": {}, "K": {}, "Q": {}, "C": {}}
    cur = None
    with open(path) as f:
        for line in f:
            if line.startswith("#"):
                continue
            tag, _, rest = line.partition(" ")
            p = rest.split()
            if tag == "n":
                d["n"] = int(p[0])
            elif tag == "V":
                d["h"] = np.array(p, dtype=float)
            elif tag == "H":
                cur = {"fS": float(p[1]), "m": int(p[2])}
                d["sets"][int(p[0])] = cur
            elif tag == "M":
                cur["members"] = np.array(p, dtype=int)
            elif tag == "O":
                cur["order"] = np.array(p, dtype=int)
            elif tag == "G":
                cur["g"] = np.array(p, dtype=float)
            elif tag == "U":
                cur["U"] = np.array(p, dtype=float)
            elif tag == "K":
                d["K"][int(p[0])] = [int(i) for i in p[1:]]
            elif tag == "Q":
                d["Q"][int(p[0])] = (float(p[1]), np.array(p[2:], dtype=int))
            elif tag == "C":
                d["C"][(int(p[0]), int(p[1]))] = np.array(p[2:], dtype=float)
    for i, s in d["sets"].items():
        for key in ("members", "order", "g", "U"):
            s.setdefault(key, np.array([], dtype=float if key in ("g", "U") else int))
        if not (len(s["order"]) == len(s["g"]) == len(s["U"]) == s["m"]):
            raise SystemExit(f"{path}: inconsistent base set {i}")
        for key in ("g", "U"):
            if not np.all(np.isfinite(s[key])):
                raise SystemExit(f"{path}: non-finite {key} in base set {i}")
    return d


def build_lp(d, prefixes, k):
    n = d["n"]
    h = d["h"]
    ncols = 1 + n + sum(d["sets"][i]["m"] for i in prefixes)
    rows, cols, vals, b = [], [], [], []
    ub = [None] + [1.0] * n + [None] * (ncols - 1 - n)
    r = 0
    rows += [r] * n; cols += list(range(1, n + 1)); vals += [1.0] * n; b.append(float(k)); r += 1  # C0
    off = 1 + n
    for i in prefixes:
        s = d["sets"][i]
        m, order, g, U = s["m"], s["order"], s["g"], s["U"]
        for t in range(m):  # C1: P_t - P_{t-1} - g_t x_{a_t} <= 0
            rows += [r, r]; cols += [off + t, 1 + order[t]]; vals += [1.0, -g[t]]
            if t > 0:
                rows.append(r); cols.append(off + t - 1); vals.append(-1.0)
            b.append(0.0); r += 1
            ub[off + t] = U[t]                                       # C2 as a bound
        # C3: z - P_m - sum_{a in S} h_a x_a <= f(S) - sum_{a in S} h_a
        mem = s["members"]
        rows.append(r); cols.append(0); vals.append(1.0)
        if m:
            rows.append(r); cols.append(off + m - 1); vals.append(-1.0)
        rows += [r] * len(mem); cols += list(1 + mem); vals += list(-h[mem])
        b.append(s["fS"] - float(h[mem].sum())); r += 1
        off += m
    A = sp.csr_matrix((vals, (rows, cols)), shape=(r, ncols))
    lb = [None] + [0.0] * n + [None] * (ncols - 1 - n)
    return A, np.array(b), list(zip(lb, ub)), r, ncols


def solve(job):
    key, k = job
    d = DATA[key]
    prefixes = d["K"][k]
    A, b, bounds, nrows, ncols = build_lp(d, prefixes, k)
    c = np.zeros(ncols)
    c[0] = -1.0
    t0 = time.perf_counter()
    res = linprog(c, A_ub=A, b_ub=b, bounds=bounds, method="highs")
    dt = 1000 * (time.perf_counter() - t0)
    val = -res.fun if res.status == 0 else math.nan
    return key, k, val, res.status, res.message, nrows, ncols, dt


def certificate(d, k, prefixes):
    """violations of C0-C3 at the proof's point for the exported optimal set O"""
    fO, O = d["Q"][k]
    x = np.zeros(d["n"])
    x[O] = 1.0
    out = []
    if len(O) > k:
        out.append(f"C0: |O|={len(O)} > k={k}")
    for i in prefixes:
        s = d["sets"][i]
        P = d["C"][(k, i)]
        prev = 0.0
        for t in range(s["m"]):
            rhs = s["g"][t] * x[s["order"][t]]
            if P[t] - prev > rhs + TOL * max(1.0, abs(rhs), abs(P[t])):
                out.append(f"C1 S=prefix{i} i={t + 1}: P_i - P_(i-1) = {P[t] - prev:.10g} > f(a_i|S) x = {rhs:.10g}")
            if P[t] > s["U"][t] + TOL * max(1.0, abs(s["U"][t])):
                out.append(f"C2 S=prefix{i} i={t + 1}: P_i = {P[t]:.10g} > U_i = {s['U'][t]:.10g}")
            prev = P[t]
        Pm = P[s["m"] - 1] if s["m"] else 0.0
        mem = s["members"]
        rhs = s["fS"] + Pm - float((d["h"][mem] * (1 - x[mem])).sum())
        if fO > rhs + TOL * max(1.0, abs(rhs)):
            out.append(f"C3 S=prefix{i}: z = f(O) = {fO:.10g} > {rhs:.10g}")
    return out


def stop(msg, rows=()):
    print("HYBRID CHECK FAILED: " + msg)
    for r in rows:
        print("  row: " + ", ".join(f"{a}={b}" for a, b in r.items()))
    raise SystemExit(1)


def process(path, lp_dir, jobs):
    with open(path) as f:
        recs = list(csv.DictReader(f))
    fields = list(recs[0].keys())
    DATA.clear()
    t_read = time.time()
    for r in recs:
        key = (r["instance"], r["seed"])
        if key not in DATA:
            DATA[key] = read_hybrid(os.path.join(lp_dir, f"{key[0]}__seed{key[1]}.hybrid.txt"))
    t_read = time.time() - t_read
    todo = [((r["instance"], r["seed"]), int(r["k"])) for r in recs]
    t0 = time.time()
    with mp.get_context("fork").Pool(jobs) as pool:
        out = {(key, k): rest for key, k, *rest in pool.imap_unordered(solve, todo)}
    wall = time.time() - t0

    cert_rows = 0
    for r in recs:
        key, k = (r["instance"], r["seed"]), int(r["k"])
        val, status, message, nrows, ncols, dt = out[(key, k)]
        if status != 0:
            stop(f"{path} instance={key[0]} seed={key[1]} k={k}: HiGHS status {status} ({message})", [r])
        r.update(hybrid_bound=f"{val:.10g}", best_bound=f"{min(val, num(r['dual_bound'])):.10g}",
                 hybrid_status="optimal", hybrid_rows=nrows, hybrid_cols=ncols, time_hybrid_solve_ms=f"{dt:.1f}")
        opt, lp = num(r["opt"]), num(r["lp_bound"])
        if not math.isnan(opt) and not (val >= opt - TOL * max(1.0, abs(opt))):
            stop(f"check 1 (validity) {path} instance={key[0]} seed={key[1]} k={k}: hybrid={val} < OPT={opt}", [r])
        if not (val <= lp + TOL * max(1.0, abs(lp))):
            stop(f"check 2 (dominance) {path} instance={key[0]} seed={key[1]} k={k}: hybrid={val} > B2={lp}", [r])
        d = DATA[key]
        if k in d["Q"]:
            viol = certificate(d, k, d["K"][k])
            if viol:
                stop(f"check 3 (certificate) {path} instance={key[0]} seed={key[1]} k={k}:\n    " +
                     "\n    ".join(viol[:20]), [r])
            r["hybrid_certificate"] = "pass"
            cert_rows += 1
        else:
            r["hybrid_certificate"] = "n/a"

    # Phase 2: bound_k <- min_{k' >= k} bound_k' within each (instance, seed)
    groups = {}
    for r in recs:
        groups.setdefault((r["instance"], r["seed"]), []).append(r)
    for g in groups.values():
        g.sort(key=lambda r: int(r["k"]))
        for col in ("dual_bound", "lp_bound", "hybrid_bound", "best_bound"):
            run = math.inf
            for r in reversed(g):
                run = min(run, num(r[col]))
                r[col + "_mono"] = f"{run:.10g}"
    mono_checked = 0
    for r in recs:
        opt = num(r["opt"])
        if math.isnan(opt):
            continue
        mono_checked += 1
        for col in ("dual_bound_mono", "lp_bound_mono", "hybrid_bound_mono", "best_bound_mono"):
            if not (num(r[col]) >= opt - TOL * max(1.0, abs(opt))):
                stop(f"check 1 on {col} {path} seed={r['seed']} k={r['k']}: {r[col]} < OPT={opt}", [r])

    new = ["hybrid_bound", "best_bound", "hybrid_status", "hybrid_rows", "hybrid_cols", "time_hybrid_solve_ms",
           "hybrid_certificate", "dual_bound_mono", "lp_bound_mono", "hybrid_bound_mono", "best_bound_mono"]
    fields += [c for c in new if c not in fields]
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(recs)
    per_inst = {}
    for (key, k), rest in out.items():
        per_inst[key] = per_inst.get(key, 0) + rest[-1]
    worst = max(per_inst.values()) / 1000
    print(f"  {path}: {len(recs)} LPs optimal (wall {wall:.1f}s, read {t_read:.1f}s, max solve time per "
          f"instance+seed {worst:.1f}s, largest LP {max(v[4] for v in out.values())} cols); checks: "
          f"validity 0 violations, dominance 0 violations, certificate {cert_rows} rows passed, "
          f"_mono validity {mono_checked} rows passed")


def main():
    jobs = int(os.environ.get("JOBS", os.cpu_count() or 1))
    for res in sys.argv[1:]:
        files = sorted(glob.glob(os.path.join(res, "*_baselines.csv")))
        for path in files:
            if os.path.basename(path).startswith("maxcut_generic"):
                continue
            process(path, os.path.join(res, "lp_export"), jobs)


if __name__ == "__main__":
    main()

"""How often are published MONOTONE upper bounds invalid on our non-monotone instances?

For every <label> in a results directory, combines
  <label>_baselines.csv            OPT, best_found, greedy, random greedy, and the controls
                                   (top_k_bound, marginal_bound, dual_bound, lp_bound, hybrid_bound)
  <label>_monoraw.csv              M1 (BQS Dual), M2 (Marginal without penalty), A2 (our DP with raw caps
  (or the same columns in the      f_S(A_i) + penalty) and the monotone flag (problem binary, --monotone-only
   baselines CSV)                  or --monotone), on exactly NM-Dual's base sets
  <label>_baselines_prefix.csv     A1 = min over NM-Dual's base sets of f(S) + DP_S (NM-Dual without penalty)
  lp_export/<inst>__seed<s>.{hybrid,mono}.txt   for M3 (Zhang-Tang-Tang Lambda^{3*}, cumulative-P LP with
                                   the monotone prefix caps P^S_i <= f_S(A_i)), solved with HiGHS:
                                     m3lit: increments P_i - P_{i-1} >= 0 (as in the paper)
                                     m3fav: increments free (larger feasible set, larger bound)
and writes <label>_violations.csv: per (seed, k) every method's bound, violated flag, severity and
misleading-certificate flags (greedy, random-greedy mean, best found exceeding the bound).

Violation: bound < OPT_k - 1e-6 max(1,|OPT_k|) on rows with OPT; otherwise certified violation:
bound < best_found - 1e-6 max(1,|best_found|) (a lower bound on the true rate).
Mandatory checks (stop on failure): controls never violate; on instances whose seed is monotone
(f(a | V - a) >= 0 for all a) M1-M3 never violate.
Fairness: an infeasible M3-literal LP (no bound produced) is recorded in m3lit_infeasible and NOT
counted as a violation. M3 is skipped for an (instance, seed) when the B5 LP of the same size took > 300 s there, or as soon as
the summed M3 solve time for that (instance, seed, variant) exceeds 300 s.

Usage: python violations.py <results_dir> [...]    env JOBS     needs numpy, scipy, pandas
"""
import math
import multiprocessing as mp
import os
import sys
import time

import numpy as np
import pandas as pd
import scipy.sparse as sp
from scipy.optimize import linprog

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hybrid_lp import read_hybrid  # noqa: E402

TOL = 1e-6
BUDGET_S = 300.0
METHODS = ["m1", "m2", "m3lit", "m3fav", "a1", "a2"]
CONTROLS = {"topk": "top_k_bound", "marginal": "marginal_bound", "nmdual": "dual_bound", "b2": "lp_bound",
            "hybrid": "hybrid_bound"}
DATA = {}


def read_mono(path):
    d = {"O": {}, "F": {}}
    with open(path) as f:
        for line in f:
            if line.startswith("#") or line.startswith("n "):
                continue
            tag, i, *rest = line.split()
            d[tag][int(i)] = np.array(rest, dtype=int if tag == "O" else float)
    return d


def build_m3(d, mono, prefixes, k, literal):
    n, h = d["n"], d["h"]
    ncols = 1 + n + sum(d["sets"][i]["m"] for i in prefixes)
    rows, cols, vals, b = [], [], [], []
    ub = [None] + [1.0] * n + [None] * (ncols - 1 - n)
    r = 0
    rows += [r] * n; cols += list(range(1, n + 1)); vals += [1.0] * n; b.append(float(k)); r += 1
    off = 1 + n
    for i in prefixes:
        s = d["sets"][i]
        m, order, g = s["m"], s["order"], s["g"]
        if not np.array_equal(order, mono["O"].get(i, np.array([], dtype=int))):
            raise SystemExit(f"ordering of prefix {i} differs between the hybrid and mono exports")
        F = mono["F"][i] if m else np.array([])
        for t in range(m):
            rows += [r, r]; cols += [off + t, 1 + order[t]]; vals += [1.0, -g[t]]
            if t > 0:
                rows.append(r); cols.append(off + t - 1); vals.append(-1.0)
            b.append(0.0); r += 1
            if literal:  # P_t - P_{t-1} >= 0
                rows.append(r); cols.append(off + t); vals.append(-1.0)
                if t > 0:
                    rows.append(r); cols.append(off + t - 1); vals.append(1.0)
                b.append(0.0); r += 1
            ub[off + t] = F[t]  # monotone prefix cap P_t <= f_S(A_t)
        mem = s["members"]
        rows.append(r); cols.append(0); vals.append(1.0)
        if m:
            rows.append(r); cols.append(off + m - 1); vals.append(-1.0)
        rows += [r] * len(mem); cols += list(1 + mem); vals += list(-h[mem])
        b.append(s["fS"] - float(h[mem].sum())); r += 1
        off += m
    A = sp.csr_matrix((vals, (rows, cols)), shape=(r, ncols))
    lb = [None] + [0.0] * n + [None] * (ncols - 1 - n)
    return A, np.array(b), list(zip(lb, ub))


def solve(job):
    key, k, literal = job
    d, mono = DATA[key]
    A, b, bounds = build_m3(d, mono, d["K"][k], k, literal)
    c = np.zeros(A.shape[1])
    c[0] = -1.0
    t0 = time.perf_counter()
    res = linprog(c, A_ub=A, b_ub=b, bounds=bounds, method="highs")
    return key, k, literal, (-res.fun if res.status == 0 else math.nan), res.status, res.message, \
        1000 * (time.perf_counter() - t0)


def solve_m3(groups, lp_dir, hybrid_times, jobs):
    """{(instance, seed): {k: (m3lit, m3fav)}}, a status per (instance, seed, variant), and the set of
    (instance, seed, k, literal) whose LP is infeasible (possible for the literal version: increments >= 0
    force P^S_i >= 0, while the cap f_S(A_i) can be negative on a non-monotone f)"""
    out, status, infeasible = {}, {}, set()
    for (inst, seed), ks in groups.items():
        key = (inst, str(seed))
        out[key] = {k: [math.nan, math.nan] for k in ks}
        if hybrid_times.get(key, 0) > BUDGET_S * 1000:
            for lit in (True, False):
                status[(key, lit)] = f"skipped: B5 of the same size took {hybrid_times[key] / 1000:.0f} s"
            continue
        DATA.clear()
        DATA[key] = (read_hybrid(os.path.join(lp_dir, f"{inst}__seed{seed}.hybrid.txt")),
                     read_mono(os.path.join(lp_dir, f"{inst}__seed{seed}.mono.txt")))
        for lit in (True, False):
            spent, skipped = 0.0, False
            pool = mp.get_context("fork").Pool(jobs)
            try:
                for key_, k, _, val, st, msg, dt in pool.imap_unordered(solve, [(key, k, lit) for k in ks]):
                    if st == 2:  # infeasible: the method produces no bound for this row
                        infeasible.add((key, k, lit))
                        spent += dt
                        continue
                    if st != 0:
                        raise SystemExit(f"M3 ({'literal' if lit else 'favorable'}) {inst} seed {seed} k={k}: "
                                         f"HiGHS status {st} ({msg})")
                    out[key][k][0 if lit else 1] = val
                    spent += dt
                    if spent > BUDGET_S * 1000:
                        skipped = True
                        break
            finally:
                pool.terminate()
            if skipped:
                for k in ks:
                    out[key][k][0 if lit else 1] = math.nan
                status[(key, lit)] = f"skipped: > {BUDGET_S:.0f} s of solves"
            else:
                status[(key, lit)] = f"solved ({spent / 1000:.1f} s)"
    return out, status, infeasible


def stop(msg, df=None):
    print("VIOLATIONS CHECK FAILED: " + msg)
    if df is not None:
        print(df.to_string())
    raise SystemExit(1)


def process(res, label, jobs):
    base = pd.read_csv(os.path.join(res, f"{label}_baselines.csv"))
    mono_label = "maxcut_generic" if label.startswith("k_sweep_n20") else label
    mono_path = os.path.join(res, f"{mono_label}_monoraw.csv")
    key = ["seed", "k"]
    if os.path.exists(mono_path):
        mono = pd.read_csv(mono_path)
        m = base.merge(mono[key + ["greedy", "m1_bound", "m2_bound", "a2_bound", "monotone_flag", "min_f_a_V_minus_a"]],
                       on=key, suffixes=("", "_mono"))
        if len(m) != len(base) or not np.allclose(m["greedy"], m["greedy_mono"], rtol=1e-9, atol=1e-9):
            stop(f"{label}: the monotone run is not on the same instances (greedy values differ)")
    else:
        m = base.copy()
    pre = pd.read_csv(os.path.join(res, f"{mono_label}_baselines_prefix.csv"))
    a1 = (pre.assign(v=pre["f_S"] + pre["dual_S"]).groupby(key)["v"].min().rename("a1_bound").reset_index())
    m = m.merge(a1, on=key, how="left")
    if "problem" not in m:  # the original max-cut CSV
        m["problem"] = "max_cut"
    if "best_found" not in m:
        m["best_found"] = m[["opt", "greedy", "rg_max"]].max(axis=1)

    # M3
    inst = m["instance"].iloc[0]
    groups = {(inst, s): sorted(g["k"].tolist()) for s, g in m.groupby("seed")}
    htimes = {(inst, str(s)): g["time_hybrid_solve_ms"].sum() for s, g in m.groupby("seed")}
    t0 = time.time()
    m3, status, infeasible = solve_m3(groups, os.path.join(res, "lp_export"), htimes, jobs)
    m["m3lit_bound"] = [m3[(inst, str(s))][k][0] for s, k in zip(m["seed"], m["k"])]
    m["m3fav_bound"] = [m3[(inst, str(s))][k][1] for s, k in zip(m["seed"], m["k"])]
    m["m3lit_status"] = [status[((inst, str(s)), True)] for s in m["seed"]]
    m["m3fav_status"] = [status[((inst, str(s)), False)] for s in m["seed"]]
    m["m3lit_infeasible"] = [int(((inst, str(s)), k, True) in infeasible) for s, k in zip(m["seed"], m["k"])]
    m["m3fav_infeasible"] = [int(((inst, str(s)), k, False) in infeasible) for s, k in zip(m["seed"], m["k"])]

    out = m[["problem", "instance", "n", "seed", "k", "opt", "best_found", "greedy", "rg_mean",
             "monotone_flag", "min_f_a_V_minus_a", "m3lit_status", "m3fav_status", "m3lit_infeasible",
             "m3fav_infeasible"]].copy()
    out.insert(1, "label", label)
    has_opt = m["opt"].notna()
    ref = m["opt"].where(has_opt, m["best_found"])
    tol = TOL * ref.abs().clip(lower=1)
    out["ref_kind"] = np.where(has_opt, "opt", "best_found")
    for name, col in [(x, f"{x}_bound") for x in METHODS] + list(CONTROLS.items()):
        bnd = m[col]
        out[f"{name}_bound"] = bnd
        viol = (bnd < ref - tol) & bnd.notna()
        out[f"{name}_violated"] = np.where(bnd.isna(), np.nan, viol.astype(float))
        out[f"{name}_severity"] = np.where(viol, (ref - bnd) / ref.abs(), np.where(bnd.isna(), np.nan, 0.0))
        for alg in ("greedy", "rg_mean", "best_found"):
            out[f"{name}_mislead_{alg}"] = np.where(bnd.isna(), np.nan,
                                                    (m[alg] > bnd * (1 + 1e-9) + 1e-12).astype(float))

    # mandatory checks
    for name in CONTROLS:
        bad = out[out[f"{name}_violated"] == 1]
        if len(bad):
            stop(f"{label}: control {name} violated on {len(bad)} rows", bad[["seed", "k", "opt", "best_found",
                                                                             f"{name}_bound"]])
    mono_rows = out[out["monotone_flag"] == 1]
    for name in ("m1", "m2", "m3lit", "m3fav"):
        bad = mono_rows[mono_rows[f"{name}_violated"] == 1]
        if len(bad):
            stop(f"{label}: {name} violated on a MONOTONE instance (implementation bug?)",
                 bad[["seed", "k", "opt", "best_found", f"{name}_bound"]])

    path = os.path.join(res, f"{label}_violations.csv")
    out.to_csv(path, index=False)
    rate = {x: out[f"{x}_violated"].mean() for x in METHODS}
    print(f"  {path}: {len(out)} rows, monotone seeds {int(out.groupby('seed')['monotone_flag'].first().sum())}/"
          f"{out['seed'].nunique()}; violation rate " + ", ".join(f"{x} {v:.0%}" for x, v in rate.items() if not math.isnan(v))
          + f"; M3 {sorted(set(status.values()))[:2]} ({time.time() - t0:.0f}s)", flush=True)
    return out


def main():
    """python violations.py <results_dir> [...] [--lambda-out PATH]
    --lambda-out: the directories hold the diverse-rec lambda sweep (labels ml20_lambda<L>); write all rows
    with a lambda column to PATH and require 0 violations of every method at lambda = 0."""
    jobs = int(os.environ.get("JOBS", os.cpu_count() or 1))
    args = sys.argv[1:]
    lam_out = None
    if "--lambda-out" in args:
        i = args.index("--lambda-out")
        lam_out = args[i + 1]
        del args[i:i + 2]
    frames = []
    for res in args:
        labels = sorted(f[: -len("_baselines.csv")] for f in os.listdir(res)
                        if f.endswith("_baselines.csv") and not f.startswith("maxcut_generic"))
        for label in labels:
            frames.append(process(res, label, jobs))
    if lam_out:
        allv = pd.concat(frames, ignore_index=True)
        allv.insert(2, "lambda", allv["label"].str.extract(r"lambda([0-9.]+)")[0].astype(float))
        zero = allv[allv["lambda"] == 0]
        for name in METHODS + list(CONTROLS):
            bad = zero[zero[f"{name}_violated"] == 1]
            if len(bad):
                stop(f"lambda = 0 (monotone, modular): {name} violated on {len(bad)} rows",
                     bad[["seed", "k", "opt", f"{name}_bound"]])
        allv.sort_values(["lambda", "seed", "k"]).to_csv(lam_out, index=False)
        print(f"wrote {lam_out}: {len(allv)} rows; lambda = 0: 0 violations for every method")


if __name__ == "__main__":
    main()

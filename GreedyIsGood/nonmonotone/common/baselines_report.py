"""Tables for baselines_report.md from the *_baselines.csv results (after common/baselines_lp.py).

Sections: instance-identity check against the original CSVs, validity checks, mean bound/OPT per
k on instances with OPT for every k, tightest bound per k (and where NM-Dual wins/loses), bounds
relative to the best solution found on the larger instances, runtimes, lattice sizes.

Usage: python common/baselines_report.py --out baselines_report_tables.md     (needs pandas)
"""
import argparse
import glob
import math
import os

import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROBLEMS = [("max_cut", "src"), ("directed_cut", "directed_cut"), ("revenue_max", "revenue_max"),
            ("diverse_rec", "diverse_recommendation"), ("log_det", "log_determinant"),
            ("gaussian_mi", "gaussian_mi"), ("hypergraph_cut", "hypergraph_cut")]
BOUNDS = {"dual_bound": "NM-Dual", "top_k_bound": "Top-k", "total": "Total", "lp_bound": "LP (B2)",
          "gamma1_bound": "γ1 (B3)", "gamma1_noprune": "γ1 no-prune", "mu2_bound": "μ2 (B4)",
          "mu3_bound": "μ3 (B4)"}
NEW = ["lp_bound", "gamma1_bound", "gamma1_noprune", "mu2_bound", "mu3_bound"]
SAME_COLS = ["opt", "top_k_bound", "total_bound", "dual_bound", "dual_bound_S0", "greedy", "rg_mean",
             "rg_std", "rg_min", "rg_max", "best_found"]


def load(problem, folder):
    out = []
    for f in sorted(glob.glob(os.path.join(ROOT, folder, "results", "*_baselines.csv"))):
        if os.path.basename(f).startswith("maxcut_generic"):
            continue
        df = pd.read_csv(f)
        df["total"] = df["total_bound"] if "total_bound" in df else df["total_weight"]
        label = os.path.basename(f)[: -len("_baselines.csv")]
        out.append((label, df, f[: -len("_baselines.csv")] + ".csv"))
    return out


def md(df, floatfmt="{:.3f}"):
    cols = list(df.columns)
    lines = ["| " + " | ".join(map(str, cols)) + " |", "|" + "---|" * len(cols)]
    ints = {c for c in cols if pd.api.types.is_integer_dtype(df[c])}
    for _, r in df.iterrows():
        cells = []
        for c in cols:
            v = r[c]
            if c in ints:
                cells.append(str(int(v)))
            elif isinstance(v, float):
                cells.append("–" if math.isnan(v) else floatfmt.format(v))
            else:
                cells.append(str(v))
        lines.append("| " + " | ".join(cells) + " |")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "baselines_report_tables.md"))
    args = ap.parse_args()
    data = {p: load(p, f) for p, f in PROBLEMS}
    S = []

    # ---- identity with the original runs
    S.append("## Same instances as the original results\n")
    rows = []
    for p, insts in data.items():
        for label, df, orig_path in insts:
            if p == "max_cut":
                rows.append({"problem": p, "instance": label, "rows": len(df),
                             "max rel. diff (existing columns)": 0.0,
                             "note": "existing columns copied from the original CSV; OPT/top-k/greedy/dual "
                                     "re-derived on the same graphs and checked equal"})
                continue
            o = pd.read_csv(orig_path)
            keys = ["seed", "k"]
            m = o.merge(df, on=keys, suffixes=("_o", "_b"))
            worst = 0.0
            for c in SAME_COLS:
                a, b = m[c + "_o"], m[c + "_b"]
                d = ((a - b).abs() / a.abs().clip(lower=1)).where(~(a.isna() & b.isna()), 0)
                worst = max(worst, float(d.max()))
            rows.append({"problem": p, "instance": label, "rows": len(df),
                         "max rel. diff (existing columns)": worst, "note": f"{len(m)}/{len(o)} rows matched"})
    S.append(md(pd.DataFrame(rows), "{:.1e}") + "\n")

    # ---- validity
    S.append("## Validity checks\n")
    rows = []
    for p, insts in data.items():
        n_rows = n_opt = viol = lat_checked = lat_bad = 0
        for label, df, _ in insts:
            n_rows += len(df)
            w = df[df["opt"].notna()]
            n_opt += len(w)
            tol = 1e-6 * w["opt"].abs().clip(lower=1)
            for b in NEW:
                viol += int((w[b] < w["opt"] - tol).sum())
            lc = df[df["opt_unconstrained"].notna()].drop_duplicates(["seed"])
            lat_checked += len(lc)
            ltol = 1e-6 * lc["opt_unconstrained"].abs().clip(lower=1)  # same tolerance as the checker
            lat_bad += int((lc["lattice_opt_unconstrained"] < lc["opt_unconstrained"] - ltol).sum())
            lat_bad += int((df["lattice_A_size"] > df["lattice_B_size"]).sum())
        rows.append({"problem": p, "rows": n_rows, "rows with OPT": n_opt,
                     "bound checks (5 per row)": 5 * n_opt, "violations": viol,
                     "lattice checks (instances)": lat_checked, "lattice violations": lat_bad})
    S.append(md(pd.DataFrame(rows), "{:.0f}") + "\n")

    # ---- bound / OPT per k on instances with OPT for all k
    S.append("## Mean bound / OPT per k (instances with OPT at every k; mean over seeds)\n")
    tight_rows = []
    for p, insts in data.items():
        for label, df, _ in insts:
            if not df["opt"].notna().all():
                continue
            cols = [b for b in BOUNDS if b in df and df[b].notna().any()]
            g = df.assign(**{b: df[b] / df["opt"] for b in cols}).groupby("k")[cols].mean()
            g = g.rename(columns=BOUNDS).reset_index()
            S.append(f"### {p} / {label} (n={df['n'].iloc[0]}, {df['seed'].nunique()} seeds)\n")
            S.append(md(g) + "\n")
            main_cols = [BOUNDS[b] for b in cols if b != "gamma1_noprune"]
            for _, r in g.iterrows():
                vals = {c: r[c] for c in main_cols if not math.isnan(r[c])}
                best = min(vals, key=vals.get)
                others = {c: v for c, v in vals.items() if c != "NM-Dual"}
                rival = min(others, key=others.get)
                tight_rows.append({"problem": p, "instance": label, "k": int(r["k"]), "tightest": best,
                                   "NM-Dual/OPT": vals["NM-Dual"], "best other": rival,
                                   "best other/OPT": others[rival]})

    S.append("## Tightest bound per k (instances with OPT)\n")
    t = pd.DataFrame(tight_rows)
    summ = []
    for (p, label), g in t.groupby(["problem", "instance"], sort=False):
        wins = g[g["tightest"] == "NM-Dual"]["k"].tolist()
        lose = g[g["tightest"] != "NM-Dual"]
        desc = "; ".join(f"k={r.k}: {r.tightest} ({r['best other/OPT']:.3f} vs {r['NM-Dual/OPT']:.3f})"
                         for _, r in lose.iterrows())
        summ.append({"problem": p, "instance": label, "NM-Dual tightest at k": compress(wins),
                     "NM-Dual beaten at": desc or "never"})
    S.append(md(pd.DataFrame(summ)) + "\n")

    # ---- larger instances: bound / best found
    S.append("## Larger instances: mean bound / best solution found (no OPT; best found <= OPT)\n")
    rows = []
    for p, insts in data.items():
        for label, df, _ in insts:
            if df["opt"].notna().all():
                continue
            cols = [b for b in BOUNDS if b in df and df[b].notna().any()]
            r = {"problem": p, "instance": label, "n": int(df["n"].iloc[0]),
                 "k": f"{df['k'].min()}–{df['k'].max()}"}
            for b in cols:
                r[BOUNDS[b]] = float((df[b] / df["best_found"]).mean())
            ratio = df[[b for b in cols if b not in ("dual_bound", "gamma1_noprune")]].min(axis=1)
            r["NM-Dual tightest (rows)"] = f"{(df['dual_bound'] <= ratio + 1e-9).mean():.0%}"
            rows.append(r)
    S.append(md(pd.DataFrame(rows)) + "\n")

    # ---- runtime
    S.append("## Runtime per bound (ms, mean per instance and seed; LP = export + solve per k)\n")
    rows = []
    for p, insts in data.items():
        for label, df, _ in insts:
            per_seed = df.groupby("seed").agg(dual=("time_dual_ms", "max"), lp_export=("time_lp_export_ms", "first"),
                                              lp_solve=("time_lp_ms", "sum"), gamma1=("time_gamma1_ms", "first"),
                                              mu=("time_mu_ms", "first"))
            r = {"problem": p, "instance": label, "n": int(df["n"].iloc[0])}
            r.update({c: float(v) for c, v in per_seed.mean().items()})
            r["LP solve per k"] = float(df["time_lp_ms"].mean())
            rows.append(r)
    S.append(md(pd.DataFrame(rows), "{:.1f}") + "\n")
    S.append("`dual` = NM-Dual for the largest k (all base sets), `lp_export` = computing the LP rows for all "
             "base sets, `lp_solve` = HiGHS over all k, `gamma1` = Iterative Prune + both double greedies, "
             "`mu` = μ2/μ3 for all candidates.\n")

    # ---- lattice
    S.append("## Iterative Prune lattice sizes\n")
    rows = []
    for p, insts in data.items():
        for label, df, _ in insts:
            g = df.drop_duplicates("seed")
            n = int(df["n"].iloc[0])
            rows.append({"problem": p, "instance": label, "n": n, "mean |A*|": g["lattice_A_size"].mean(),
                         "mean |B*|": g["lattice_B_size"].mean(),
                         "free |B*\\A*| / n": ((g["lattice_B_size"] - g["lattice_A_size"]) / n).mean(),
                         "prune rounds": g["prune_rounds"].mean(),
                         "γ1 / γ1 no-prune": (g["gamma1_bound"] / g["gamma1_noprune"]).mean()})
    S.append(md(pd.DataFrame(rows), "{:.2f}") + "\n")

    # ---- OPT_k inside the lattice (informational)
    rows = []
    for p, insts in data.items():
        for label, df, _ in insts:
            w = df[df["lattice_opt_k"].notna() & df["opt"].notna()]
            if w.empty:
                continue
            inside = (w["lattice_opt_k"] >= w["opt"] - 1e-6 * w["opt"].abs().clip(lower=1)).mean()
            rows.append({"problem": p, "instance": label, "rows": len(w),
                         "an optimal size-≤k set lies in [A*, B*]": f"{inside:.0%}"})
    if rows:
        S.append("## Informational: cardinality-constrained optima inside the lattice\n")
        S.append(md(pd.DataFrame(rows)) + "\n")

    with open(args.out, "w") as f:
        f.write("\n".join(S))
    print(f"wrote {args.out}")


def compress(ks):
    """[1,2,3,5,6] -> '1–3, 5–6'"""
    if not ks:
        return "none"
    out, start, prev = [], ks[0], ks[0]
    for k in ks[1:] + [None]:
        if k is not None and k == prev + 1:
            prev = k
            continue
        out.append(f"{start}" if start == prev else f"{start}–{prev}")
        if k is not None:
            start = prev = k
    return ", ".join(out)


if __name__ == "__main__":
    main()

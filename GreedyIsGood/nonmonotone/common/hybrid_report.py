"""Tables for hybrid_report.md from the *_baselines.csv results after common/hybrid_lp.py.

Every average excludes k = 1 (all bounds are exact there). The reference `ref` is OPT on instances
where brute force gave OPT for every k, otherwise the best solution found (<= OPT).

Usage: python common/hybrid_report.py --out hybrid_report_tables.md      (needs pandas)
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
ABL = {"marginal_bound": "Marginal", "dual_bound": "NM-Dual", "lp_bound": "B2", "hybrid_bound": "Hybrid",
       "best_bound": "min(NM-Dual, Hybrid)"}


def md(df, fmt="{:.3f}"):
    cols = list(df.columns)
    ints = {c for c in cols if pd.api.types.is_integer_dtype(df[c])}
    out = ["| " + " | ".join(map(str, cols)) + " |", "|" + "---|" * len(cols)]
    for _, r in df.iterrows():
        cells = []
        for c in cols:
            v = r[c]
            if c in ints:
                cells.append(str(int(v)))
            elif isinstance(v, float):
                cells.append("–" if math.isnan(v) else fmt.format(v))
            else:
                cells.append(str(v))
        out.append("| " + " | ".join(cells) + " |")
    return "\n".join(out)


def load():
    data = []
    for prob, folder in PROBLEMS:
        for f in sorted(glob.glob(os.path.join(ROOT, folder, "results", "*_baselines.csv"))):
            if os.path.basename(f).startswith("maxcut_generic"):
                continue
            df = pd.read_csv(f)
            label = os.path.basename(f)[: -len("_baselines.csv")]
            ref_opt = df["opt"].notna().all()
            df["ref"] = df["opt"] if ref_opt else df["best_found"]
            data.append((prob, label, df, ref_opt))
    return data


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "hybrid_report_tables.md"))
    args = ap.parse_args()
    data = load()
    S = []

    # 1. checks
    S.append("## 1. Checks\n")
    rows = []
    for prob in dict.fromkeys(p for p, *_ in data):
        g = [(l, d) for p, l, d, _ in data if p == prob]
        all_ = pd.concat([d for _, d in g])
        w = all_[all_["opt"].notna()]
        tol = 1e-6 * w["opt"].abs().clip(lower=1)
        tol_lp = 1e-6 * all_["lp_bound"].abs().clip(lower=1)
        mono_viol = sum(int((w[c] < w["opt"] - tol).sum()) for c in
                        ("dual_bound_mono", "lp_bound_mono", "hybrid_bound_mono", "best_bound_mono"))
        rows.append({"problem": prob, "rows (LPs)": len(all_),
                     "non-optimal solves": int((all_["hybrid_status"] != "optimal").sum()),
                     "check 1 rows (OPT)": len(w), "check 1 violations": int((w["hybrid_bound"] < w["opt"] - tol).sum()),
                     "check 2 rows": len(all_),
                     "check 2 violations": int((all_["hybrid_bound"] > all_["lp_bound"] + tol_lp).sum()),
                     "check 3 rows (certificate)": int((all_["hybrid_certificate"] == "pass").sum()),
                     "check 3 failures": int((all_["hybrid_certificate"] == "fail").sum()),
                     "_mono check 1 (4 bounds)": 4 * len(w), "_mono violations": mono_viol})
    S.append(md(pd.DataFrame(rows), "{:.0f}") + "\n")
    S.append("Check 3 builds the proof's feasible point (x = 1_O, z = f(O), P^S_i = f_S(O' in the first i "
             "positions)) for the brute-force optimal set O and verifies C0–C3; it ran on every row where "
             "brute force produced an optimal set.\n")

    # 2. ablation summary (k > 1)
    S.append("## 2. 2×2 ablation: mean bound / ref over k > 1\n")
    S.append("Columns: Marginal (no LP coupling, no caps), NM-Dual (caps, no coupling), B2 (coupling, no caps), "
             "Hybrid (coupling + caps), and min(NM-Dual, Hybrid).\n")
    rows = []
    for prob, label, df, ref_opt in data:
        d = df[df["k"] > 1]
        r = {"problem": prob, "instance": label, "n": int(df["n"].iloc[0]), "ref": "OPT" if ref_opt else "best"}
        for c, name in ABL.items():
            r[name] = float((d[c] / d["ref"]).mean())
        tight = {name: r[name] for name in ABL.values()}
        r["tightest"] = min(tight, key=tight.get)
        rows.append(r)
    S.append(md(pd.DataFrame(rows)) + "\n")

    # 3. caps tighten the LP
    S.append("## 3. How much do the caps tighten the LP? (B2 − Hybrid) / B2, rows with k > 1\n")
    rows = []
    for prob in dict.fromkeys(p for p, *_ in data):
        d = pd.concat([df[df["k"] > 1] for p, _, df, _ in data if p == prob])
        red = (d["lp_bound"] - d["hybrid_bound"]) / d["lp_bound"]
        rows.append({"problem": prob, "rows": len(d), "mean": float(red.mean()), "max": float(red.max()),
                     "rows Hybrid < B2 by > 1%": f"{(red > 0.01).mean():.0%}"})
    S.append(md(pd.DataFrame(rows), "{:.2%}") + "\n")
    rows = []
    for prob, label, df, _ in data:
        d = df[df["k"] > 1]
        red = (d["lp_bound"] - d["hybrid_bound"]) / d["lp_bound"]
        rows.append({"problem": prob, "instance": label, "mean": float(red.mean()), "max": float(red.max()),
                     "rows > 1%": f"{(red > 0.01).mean():.0%}"})
    S.append("Per instance:\n\n" + md(pd.DataFrame(rows), "{:.2%}") + "\n")

    # 4. where min(NM-Dual, Hybrid) beats both NM-Dual and B2
    S.append("## 4. Where min(NM-Dual, Hybrid) beats both NM-Dual alone and B2 alone (k > 1)\n")
    S.append("Gain = (min(NM-Dual, B2) − min(NM-Dual, Hybrid)) / min(NM-Dual, B2); a row counts when the gain "
             "exceeds 1e-6. Since Hybrid ≤ B2, this happens exactly when Hybrid < NM-Dual and Hybrid < B2.\n")
    rows = []
    for prob, label, df, _ in data:
        d = df[df["k"] > 1]
        prev = d[["dual_bound", "lp_bound"]].min(axis=1)
        gain = (prev - d["best_bound"]) / prev
        win = gain > 1e-6
        rows.append({"problem": prob, "instance": label, "rows": len(d), "rows beating both": f"{win.mean():.0%}",
                     "mean gain (winning rows)": float(gain[win].mean()) if win.any() else math.nan,
                     "max gain": float(gain.max()),
                     "k range of wins": f"{d.loc[win, 'k'].min()}–{d.loc[win, 'k'].max()}" if win.any() else "–"})
    S.append(md(pd.DataFrame(rows), "{:.2%}") + "\n")

    # 5. Phase 2 (monotone in k)
    S.append("## 5. Phase 2: bound_k ← min over k' ≥ k (same instance and seed), rows with k > 1\n")
    rows = []
    for prob in dict.fromkeys(p for p, *_ in data):
        d = pd.concat([df[df["k"] > 1] for p, _, df, _ in data if p == prob])
        r = {"problem": prob, "rows": len(d)}
        for c, name in (("dual_bound", "NM-Dual"), ("lp_bound", "B2"), ("hybrid_bound", "Hybrid"),
                        ("best_bound", "min")):
            red = (d[c] - d[c + "_mono"]) / d[c]
            r[f"{name}: rows changed"] = f"{(red > 1e-9).mean():.0%}"
            r[f"{name}: mean / max reduction"] = f"{red.mean():.2%} / {red.max():.2%}"
        rows.append(r)
    S.append(md(pd.DataFrame(rows)) + "\n")

    # 6. LP sizes and runtimes
    S.append("## 6. Hybrid LP sizes and runtimes\n")
    S.append("Per instance (mean over seeds): largest LP (rows × columns), hybrid export (C++: caps "
             "recomputation and file), HiGHS time summed over all k and per LP, NM-Dual time for the largest k, "
             "and B2 solve time summed over all k.\n")
    rows = []
    for prob, label, df, _ in data:
        per_seed = df.groupby("seed").agg(export=("time_hybrid_export_ms", "first"),
                                          solve=("time_hybrid_solve_ms", "sum"), dual=("time_dual_ms", "max"),
                                          b2=("time_lp_ms", "sum"))
        m = per_seed.mean() / 1000
        rows.append({"problem": prob, "instance": label, "n": int(df["n"].iloc[0]),
                     "max rows": int(df["hybrid_rows"].max()), "max cols": int(df["hybrid_cols"].max()),
                     "export [s]": float(m["export"]), "hybrid solve, all k [s]": float(m["solve"]),
                     "per LP [s]": float(df["time_hybrid_solve_ms"].mean() / 1000),
                     "NM-Dual [s]": float(m["dual"]), "B2 solve, all k [s]": float(m["b2"])})
    S.append(md(pd.DataFrame(rows), "{:.2f}") + "\n")

    # appendix: per-k ablation
    S.append("## Appendix: per-k ablation (mean bound / ref over seeds; k = 1 omitted)\n")
    for prob, label, df, ref_opt in data:
        d = df[df["k"] > 1]
        g = d.assign(**{c: d[c] / d["ref"] for c in ABL}).groupby("k")[list(ABL)].mean().rename(columns=ABL)
        S.append(f"### {prob} / {label} (n={df['n'].iloc[0]}, ref = {'OPT' if ref_opt else 'best found'})\n")
        S.append(md(g.reset_index()) + "\n")

    with open(args.out, "w") as f:
        f.write("\n".join(S))
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()

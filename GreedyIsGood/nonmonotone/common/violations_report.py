"""Tables for violations_report.md from the *_violations.csv files (common/violations.py).

Usage: python common/violations_report.py --out violations_report_tables.md     (needs pandas)
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
METHODS = {"m1": "M1 BQS Dual", "m2": "M2 Marginal (no pen.)", "m3lit": "M3 literal", "m3fav": "M3 favorable",
           "a1": "A1 caps, no pen.", "a2": "A2 pen., raw caps"}
CONTROLS = {"topk": "Top-k", "marginal": "Marginal", "nmdual": "NM-Dual", "b2": "B2", "hybrid": "Hybrid"}


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


def pct(x):
    return "–" if x is None or (isinstance(x, float) and math.isnan(x)) else f"{x:.1%}"


def load():
    data = {}
    for prob, folder in PROBLEMS:
        files = [f for f in glob.glob(os.path.join(ROOT, folder, "results", "*_violations.csv"))
                 if not os.path.basename(f).startswith("lambda_sweep")]  # the lambda sweep has its own section
        if files:
            data[prob] = pd.concat([pd.read_csv(f) for f in sorted(files)], ignore_index=True)
    lam = os.path.join(ROOT, "diverse_recommendation", "results", "lambda_sweep_violations.csv")
    return data, (pd.read_csv(lam) if os.path.exists(lam) else None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(ROOT, "violations_report_tables.md"))
    args = ap.parse_args()
    data, lam = load()
    S = []

    # 1. controls and monotone sanity
    S.append("## 1. Controls and monotone-instance sanity checks\n")
    rows = []
    for prob, d in data.items():
        seeds = d.groupby(["label", "seed"])["monotone_flag"].first()
        mono = d[d["monotone_flag"] == 1]
        r = {"problem": prob, "rows": len(d), "monotone (instance, seed)": f"{int(seeds.sum())}/{len(seeds)}"}
        for c, name in CONTROLS.items():
            r[f"{name} viol."] = int((d[f"{c}_violated"] == 1).sum())
        r["M1–M3 viol. on monotone seeds"] = int(sum((mono[f"{m}_violated"] == 1).sum() for m in ("m1", "m2", "m3lit", "m3fav")))
        rows.append(r)
    if lam is not None:
        seeds = lam.groupby(["lambda", "seed"])["monotone_flag"].first()
        mono = lam[lam["monotone_flag"] == 1]
        r = {"problem": "diverse_rec λ-sweep", "rows": len(lam), "monotone (instance, seed)": f"{int(seeds.sum())}/{len(seeds)}"}
        for c, name in CONTROLS.items():
            r[f"{name} viol."] = int((lam[f"{c}_violated"] == 1).sum())
        r["M1–M3 viol. on monotone seeds"] = int(sum((mono[f"{m}_violated"] == 1).sum() for m in ("m1", "m2", "m3lit", "m3fav")))
        rows.append(r)
    S.append(md(pd.DataFrame(rows)) + "\n")

    # M3 status
    S.append("### M3 solves\n")
    rows = []
    for prob, d in data.items():
        for lbl, g in d.groupby("label"):
            st = g.drop_duplicates("seed")
            rows.append({"problem": prob, "instance": lbl, "rows": len(g),
                         "M3 literal": "; ".join(sorted(set(st["m3lit_status"]))),
                         "literal infeasible rows": int(g["m3lit_infeasible"].sum()),
                         "M3 favorable": "; ".join(sorted(set(st["m3fav_status"]))),
                         "favorable infeasible rows": int(g["m3fav_infeasible"].sum())})
    S.append(md(pd.DataFrame(rows)) + "\n")

    # 2. per method x problem
    S.append("## 2. Violation rates per method and problem\n")
    S.append("*OPT rows*: rows where brute force gave OPT_k (rate = bound < OPT_k). *Certified rows*: the other rows, "
             "where a violation is certified only when the bound is below the best solution found (a lower bound "
             "on the true rate). Rates are over the rows where the method produced a bound (M3-literal: feasible "
             "LP). Severity = (ref − bound)/ref over violated rows. *Misleading* = an algorithm's own value exceeds "
             "the bound (greedy / random-greedy mean / best found), over all rows with a bound.\n")
    rows = []
    for m, name in METHODS.items():
        for prob, d in data.items():
            have = d[d[f"{m}_violated"].notna()]
            o, c = have[have["ref_kind"] == "opt"], have[have["ref_kind"] == "best_found"]
            v = have[have[f"{m}_violated"] == 1]
            rows.append({"method": name, "problem": prob, "rows": len(have),
                         "OPT rows: rate": pct(o[f"{m}_violated"].mean()) + f" ({len(o)})",
                         "certified rows: rate": pct(c[f"{m}_violated"].mean()) + f" ({len(c)})",
                         "mean severity": float(v[f"{m}_severity"].mean()) if len(v) else math.nan,
                         "max severity": float(v[f"{m}_severity"].max()) if len(v) else math.nan,
                         "misleading greedy": pct(have[f"{m}_mislead_greedy"].mean()),
                         "misleading RG mean": pct(have[f"{m}_mislead_rg_mean"].mean()),
                         "misleading best": pct(have[f"{m}_mislead_best_found"].mean())})
    S.append(md(pd.DataFrame(rows)) + "\n")

    # 3. per k (n = 20, OPT rows)
    S.append("## 3. Where violations occur in k (n = 20 instances, rows with OPT)\n")
    S.append("Violation rate per k range, and the smallest k with any violation.\n")
    rows = []
    bins = [(1, 1), (2, 5), (6, 10), (11, 20)]
    for prob, d in data.items():
        d20 = d[(d["n"] == 20) & (d["ref_kind"] == "opt")]
        for m, name in METHODS.items():
            r = {"problem": prob, "method": name}
            for lo, hi in bins:
                g = d20[(d20["k"] >= lo) & (d20["k"] <= hi)]
                r[f"k={lo}" if lo == hi else f"k={lo}–{hi}"] = pct(g[f"{m}_violated"].mean())
            ks = d20.loc[d20[f"{m}_violated"] == 1, "k"]
            r["first k"] = int(ks.min()) if len(ks) else "–"
            rows.append(r)
    S.append(md(pd.DataFrame(rows)) + "\n")

    # 4. ablation
    S.append("## 4. Ablation of our two fixes (all rows; OPT rows / certified rows)\n")
    S.append("M1 = neither fix (monotone caps, no penalty); A1 = our caps only; A2 = penalty only (raw caps); "
             "NM-Dual = both.\n")
    rows = []
    for prob, d in data.items():
        r = {"problem": prob}
        for m, name in (("m1", "M1 (neither)"), ("a1", "A1 (caps only)"), ("a2", "A2 (penalty only)"),
                        ("nmdual", "NM-Dual (both)")):
            o = d[d["ref_kind"] == "opt"][f"{m}_violated"].mean()
            c = d[d["ref_kind"] == "best_found"][f"{m}_violated"].mean()
            r[name] = f"{pct(o)} / {pct(c)}"
        rows.append(r)
    S.append(md(pd.DataFrame(rows)) + "\n")

    # 5. lambda sweep
    if lam is not None:
        S.append("## 5. λ-sweep (diverse rec, n = 20, 10 seeds, k = 1..20, OPT for every row)\n")
        rows = []
        for L, g in lam.groupby("lambda"):
            r = {"λ": L, "monotone seeds": pct(g.groupby("seed")["monotone_flag"].first().mean())}
            for m, name in METHODS.items():
                v = g[g[f"{m}_violated"] == 1]
                sev = v[f"{m}_severity"].mean() if len(v) else 0.0
                r[name] = f"{pct(g[f'{m}_violated'].mean())} (sev {sev:.3f})" if g[f"{m}_violated"].notna().any() else "–"
            rows.append(r)
        S.append(md(pd.DataFrame(rows)) + "\n")
        S.append("Cells: violation rate (mean severity over violated rows). M3-literal: over rows with a feasible LP; "
                 "infeasible rows per λ: " + ", ".join(f"λ={L:g}: {int(g['m3lit_infeasible'].sum())}"
                                                      for L, g in lam.groupby("lambda")) + ".\n")

    with open(args.out, "w") as f:
        f.write("\n".join(S))
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()

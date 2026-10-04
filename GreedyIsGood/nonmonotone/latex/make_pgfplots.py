"""Generate pgfplots figures (LaTeX) of random greedy's certified approximation ratio vs k.

For every instance of every problem (plus the original max-cut k sweep in src/results) it writes
  data/<problem>__<instance>.dat   per-k means (and sds over seeds) of
                                   RG/dual, RG/top-k, RG/total, RG/OPT  (nan = not shown)
  plots/<problem>__<instance>.tex  one tikzpicture (width \\rgplotwidth)
  figures/<problem>.tex            a single-column figure: shared legend + all instances, 2 per row
  figures/<problem>_wide.tex       the same as a two-column figure* (4 per row)
RG is the mean value of random greedy over its trials, averaged over seeds. The total-weight
curve is kept only at the k where the total bound is within --total-max-ratio of the dual bound
(the same rule as common/plot_k_sweep.py); RG/OPT only where OPT was brute-forced for every seed.

Standard library only. Run from anywhere:  python3 latex/make_pgfplots.py
Then \\input{rgplots-preamble.tex} in the preamble and \\input{figures/<problem>.tex} in the body
(see main.tex). Set \\rgroot if the latex/ folder is not the working directory of the document.
"""
import argparse
import csv
import math
import os
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
INV_E = 1 / math.e

PROBLEMS = [  # (folder, key, title, caption detail)
    ("src", "max_cut", "Max-cut",
     "$f(S)$ = weight of edges crossing $(S, V\\setminus S)$; $G(n,p)$ with integer weights in $\\{1,\\dots,9\\}$."),
    ("directed_cut", "directed_cut", "Directed cut",
     "$f(S)$ = weight of edges leaving $S$; synthetic $G(n,p)$ with random orientations and SNAP graphs."),
    ("revenue_max", "revenue_max", "Revenue maximization",
     "$f(S)=\\sum_{i\\notin S}\\sqrt{\\sum_{j\\in S} w_{ij}}$ with $w_{ij}\\sim U(0,1)$."),
    ("diverse_recommendation", "diverse_rec", "Diverse recommendation",
     "$f(S)=\\sum_{i\\in S}\\sum_{j\\in V}s_{ij}-\\lambda\\sum_{i,j\\in S}s_{ij}$, cosine similarities of "
     "MovieLens~1M rating vectors."),
    ("log_determinant", "log_det", "Log-determinant",
     "$f(S)=\\log\\det(L_S)$, $L=\\alpha(K+10^{-3}I)$ with an RBF kernel $K$ on the features."),
    ("gaussian_mi", "gaussian_mi", "Gaussian mutual information",
     "$f(S)=I(X_S;X_{V\\setminus S})$ for a Gaussian process on random points and for the Intel Berkeley "
     "lab temperature sensors."),
    ("hypergraph_cut", "hypergraph_cut", "Hypergraph cut",
     "$f(S)$ = weight of hyperedges split by $S$; random hypergraphs and the MAG-10 co-authorship hypergraph."),
]

# instance file stem -> display name (n is appended automatically)
NAMES = {
    "max_cut": {"k_sweep_n20_p0.3": "Synthetic $G(n,0.3)$"},
    "directed_cut": {"synthetic_n20": "Synthetic $G(n,0.3)$", "synthetic_n200": "Synthetic $G(n,0.1)$",
                     "email_eu_core": "email-Eu-core", "wiki_vote": "wiki-Vote"},
    "revenue_max": {"synthetic_n20": "Synthetic $G(n,0.3)$", "synthetic_n200": "Synthetic $G(n,0.1)$",
                    "facebook": "ego-Facebook", "youtube": "YouTube subgraph"},
    "diverse_rec": {"ml20_lambda0.75": "MovieLens, $\\lambda=0.75$", "ml20_lambda1.0": "MovieLens, $\\lambda=1$",
                    "ml500_lambda0.75": "MovieLens, $\\lambda=0.75$", "ml500_lambda1.0": "MovieLens, $\\lambda=1$"},
    "log_det": {"synthetic_n20_alpha3": "Synthetic, $\\alpha=3$", "wine_n100_alpha3": "Wine, $\\alpha=3$",
                "wine_n100_alpha10": "Wine, $\\alpha=10$", "wine_n300_alpha3": "Wine, $\\alpha=3$",
                "wine_n300_alpha10": "Wine, $\\alpha=10$"},
    "gaussian_mi": {"synthetic_gp_n20": "Synthetic GP", "synthetic_gp_n100": "Synthetic GP",
                    "intel": "Intel Berkeley lab"},
    "hypergraph_cut": {"synthetic_n20": "Synthetic, $m=60$", "synthetic_n500": "Synthetic, $m=2000$",
                       "mag10_top1000": "MAG-10 co-authorship"},
}
ORDER = {  # plotting order inside a figure (small instance with OPT first)
    "directed_cut": ["synthetic_n20", "synthetic_n200", "email_eu_core", "wiki_vote"],
    "revenue_max": ["synthetic_n20", "synthetic_n200", "facebook", "youtube"],
    "diverse_rec": ["ml20_lambda0.75", "ml20_lambda1.0", "ml500_lambda0.75", "ml500_lambda1.0"],
    "log_det": ["synthetic_n20_alpha3", "wine_n100_alpha3", "wine_n100_alpha10", "wine_n300_alpha3",
                "wine_n300_alpha10"],
    "gaussian_mi": ["synthetic_gp_n20", "synthetic_gp_n100", "intel"],
    "hypergraph_cut": ["synthetic_n20", "synthetic_n500", "mag10_top1000"],
}
SERIES = [("dual", "rgdual", "RG/dual"), ("topk", "rgtopk", "RG/top-$k$"),
          ("total", "rgtotal", "RG/total"), ("opt", "rgopt", "RG/OPT")]


def num(x):
    try:
        v = float(x)
    except (TypeError, ValueError):
        return math.nan
    return v


def mean_sd(vals):
    vals = [v for v in vals if not math.isnan(v)]
    if not vals:
        return math.nan, math.nan
    m = sum(vals) / len(vals)
    sd = math.sqrt(sum((v - m) ** 2 for v in vals) / (len(vals) - 1)) if len(vals) > 1 else 0.0
    return m, sd


def load_instance(path, max_ratio):
    """per-k rows {k, n, seeds, dual, topk, total, opt (+ _sd)}; returns (rows, meta)."""
    with open(path) as f:
        recs = list(csv.DictReader(f))
    total_col = "total_bound" if "total_bound" in recs[0] else "total_weight"
    by_k = defaultdict(list)
    for r in recs:
        by_k[int(r["k"])].append(r)
    rows, stride = [], max(int(r.get("chain_stride", 1) or 1) for r in recs)
    for k in sorted(by_k):
        rs = by_k[k]
        rg = [num(r["rg_mean"]) for r in rs]
        dual = [num(r["dual_bound"]) for r in rs]
        topk = [num(r["top_k_bound"]) for r in rs]
        total = [num(r[total_col]) for r in rs]
        opt = [num(r["opt"]) for r in rs]
        row = {"k": k}
        row["dual"], row["dual_sd"] = mean_sd([a / b for a, b in zip(rg, dual)])
        row["topk"], row["topk_sd"] = mean_sd([a / b for a, b in zip(rg, topk)])
        show_total = not any(math.isnan(t) for t in total) and \
            sum(total) / len(total) <= max_ratio * sum(dual) / len(dual)
        row["total"], row["total_sd"] = mean_sd([a / b for a, b in zip(rg, total)]) if show_total else (math.nan,) * 2
        has_opt = not any(math.isnan(o) or o <= 0 for o in opt)
        row["opt"], row["opt_sd"] = mean_sd([a / b for a, b in zip(rg, opt)]) if has_opt else (math.nan,) * 2
        rows.append(row)
    meta = {"n": int(recs[0]["n"]), "seeds": len({r["seed"] for r in recs}), "stride": stride,
            "kmin": rows[0]["k"], "kmax": rows[-1]["k"]}
    return rows, meta


def fmt(v):
    return "nan" if math.isnan(v) else f"{v:.5f}"


def write_dat(path, rows, meta):
    cols = ["k", "dual", "dual_sd", "topk", "topk_sd", "total", "total_sd", "opt", "opt_sd"]
    with open(path, "w") as f:
        f.write(f"# random greedy certified ratio vs k; n={meta['n']}, {meta['seeds']} seed(s); "
                f"mean (and sd) over seeds; nan = not shown\n")
        f.write(" ".join(cols) + "\n")
        for r in rows:
            f.write(" ".join(str(r["k"]) if c == "k" else fmt(r[c]) for c in cols) + "\n")


def plot_tex(key, stem, name, rows, meta, ylabel):
    present = [s for s in SERIES if any(not math.isnan(r[s[0]]) for r in rows)]
    lines = [
        f"% {key} / {stem}: random greedy certified approximation ratio vs k (generated by make_pgfplots.py)",
        "\\begin{tikzpicture}",
        "\\begin{axis}[rgaxis,",
        f"    title={{{name} ($n={meta['n']}$)}},",
        f"    xmin={meta['kmin']}, xmax={meta['kmax']},",
    ]
    if ylabel:
        lines.append("    ylabel={Certified ratio},")
    lines.append("]")
    for col, style, _ in present:
        lines.append(f"\\addplot[{style}] table[x=k, y={col}] {{\\rgroot/data/{key}__{stem}.dat}};")
    lines.append(f"\\addplot[rgonee] coordinates {{({meta['kmin']},{INV_E:.5f}) ({meta['kmax']},{INV_E:.5f})}};")
    lines += ["\\end{axis}", "\\end{tikzpicture}"]
    return "\n".join(lines) + "\n", {s[0] for s in present}


def legend_tex(series_present, columns):
    lines = ["\\begin{tikzpicture}",
             f"\\begin{{axis}}[rglegendaxis, legend columns={columns}]"]
    for col, style, label in SERIES:
        if col in series_present:
            lines.append(f"\\addlegendimage{{{style}}}\\addlegendentry{{{label}}}")
    lines += ["\\addlegendimage{rgonee}\\addlegendentry{$1/e$}", "\\end{axis}", "\\end{tikzpicture}"]
    return "\n".join(lines)


def caption(title, detail, metas, any_total, any_opt):
    strides = sorted({m["stride"] for m in metas if m["stride"] > 1})
    parts = [f"{title}: certified approximation ratio of random greedy (RG, mean over trials) "
             f"with respect to each upper bound, as a function of the cardinality $k$. {detail}"]
    if any_opt:
        parts.append("RG/OPT uses the exact optimum (brute force) where it was computable.")
    if any_total:
        parts.append("The total-weight bound is shown only where it is within $1.5\\times$ the dual bound.")
    if strides:
        every = " or ".join(f"{s}th" for s in strides)
        parts.append(f"On the largest instances the dual bound is minimized over every {every} greedy prefix only.")
    parts.append("Curves are means over graph/sample seeds.")
    return " ".join(parts)


def figure_tex(key, title, detail, items, wide):
    per_row = 4 if wide else 2
    width = "0.245\\textwidth" if wide else "0.49\\columnwidth"
    env = "figure*" if wide else "figure"
    series = set().union(*(s for _, s, _ in items))
    metas = [m for _, _, m in items]
    out = [f"% {title}: generated by make_pgfplots.py", f"\\begin{{{env}}}[t]", "\\centering",
           f"\\setlength{{\\rgplotwidth}}{{{width}}}",
           legend_tex(series, 5 if wide else 3) + "\\\\[1pt]"]
    for i, (stem, _, _) in enumerate(items):
        sep = "\\\\" if (i + 1) % per_row == 0 and i + 1 < len(items) else ("\\hfill" if i + 1 < len(items) else "")
        out.append(f"\\input{{\\rgroot/plots/{key}__{stem}.tex}}{sep}")
    out.append(f"\\caption{{{caption(title, detail, metas, 'total' in series, 'opt' in series)}}}")
    out.append(f"\\label{{fig:rg-{key}{'-wide' if wide else ''}}}")
    out.append(f"\\end{{{env}}}")
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--total-max-ratio", type=float, default=1.5)
    args = ap.parse_args()
    for d in ("data", "plots", "figures"):
        os.makedirs(os.path.join(HERE, d), exist_ok=True)

    for folder, key, title, detail in PROBLEMS:
        if key == "max_cut":
            files = [os.path.join(ROOT, "src", "results", "k_sweep_n20_p0.3.csv")]
        else:
            res = os.path.join(ROOT, folder, "results")
            stems = ORDER[key] + sorted(s for s in (os.path.splitext(f)[0] for f in os.listdir(res)
                                                    if f.endswith(".csv") and not f.endswith("_prefix.csv"))
                                        if s not in ORDER[key])
            files = [os.path.join(res, s + ".csv") for s in stems]
        items = []
        for path in (p for p in files if os.path.exists(p)):
            stem = os.path.splitext(os.path.basename(path))[0]
            rows, meta = load_instance(path, args.total_max_ratio)
            write_dat(os.path.join(HERE, "data", f"{key}__{stem}.dat"), rows, meta)
            name = NAMES.get(key, {}).get(stem, stem.replace("_", "\\_"))
            tex, present = plot_tex(key, stem, name, rows, meta, ylabel=True)
            with open(os.path.join(HERE, "plots", f"{key}__{stem}.tex"), "w") as f:
                f.write(tex)
            items.append((stem, present, meta))
        for wide in (False, True):
            with open(os.path.join(HERE, "figures", f"{key}{'_wide' if wide else ''}.tex"), "w") as f:
                f.write(figure_tex(key, title, detail, items, wide))
        print(f"{title}: {len(items)} plots")


if __name__ == "__main__":
    main()

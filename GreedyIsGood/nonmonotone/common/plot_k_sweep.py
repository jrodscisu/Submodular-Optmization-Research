"""Plots and summary for the k-sweep CSVs written by the problem binaries (common/experiment.hpp).

For every instance CSV (results/<instance>.csv) one figure with three panels:
  1. upper bound / reference vs k for the dual, top-k singleton and total-weight bounds,
     where the reference is OPT when brute force computed it and otherwise the best solution
     found (max of plain greedy and every random-greedy run; best_found <= OPT, so
     bound / best_found over-estimates bound / OPT);
  2. certified ratio greedy / bound vs k, against the 1/e guarantee;
  3. the same for random greedy (mean over trials).
Curves are means over seeds with a +-1 sd band. The total-weight bound is drawn only at the k
where it is within --total-max-ratio of the dual bound (elsewhere it is far too loose to be
informative); objectives without such a bound (log-det, Gaussian MI) omit it.

With several --results directories it also draws an overview figure across problems and,
with --summary, writes a markdown table of per-instance statistics.

Usage:
    python plot_k_sweep.py --results results --out figures
    python plot_k_sweep.py --results */results --out figures --summary RESULTS.md
Requires: pandas, matplotlib, numpy
"""
import argparse
import glob
import math
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.ticker import FixedLocator, MaxNLocator, NullFormatter, NullLocator, ScalarFormatter

INV_E = 1 / math.e
BOUNDS = {  # column -> (label, color); color follows the bound in every figure
    "dual_bound": ("Dual bound", "#2a78d6"),
    "top_k_bound": ("Top-k singletons", "#eb6834"),
    "total_bound": ("Total weight", "#1baf7a"),
}
OPT_COLOR, REF_COLOR = "#0b0b0b", "#8a8984"
PROBLEM_NAMES = {
    "directed_cut": "Directed cut", "revenue_max": "Revenue maximization",
    "diverse_rec": "Diverse recommendation", "log_det": "Log-determinant",
    "gaussian_mi": "Gaussian mutual information", "hypergraph_cut": "Hypergraph cut",
}

plt.rcParams.update({
    "figure.dpi": 130, "savefig.dpi": 200, "font.size": 10, "axes.titlesize": 11,
    "axes.spines.top": False, "axes.spines.right": False, "axes.grid": True,
    "grid.color": "#e4e3df", "grid.linewidth": 0.6, "axes.edgecolor": "#8a8984",
    "lines.linewidth": 2, "lines.markersize": 4, "legend.frameon": False,
})


def load(path):
    df = pd.read_csv(path)
    ref_is_opt = df["opt"].notna().all()
    df["ref"] = df["best_found"]
    for b in BOUNDS:
        df[f"{b}_r"] = df[b] / df["ref"]
        for alg in ("greedy", "rg_mean"):
            df[f"{alg}_over_{b}"] = df[alg] / df[b]
    for alg in ("greedy", "rg_mean"):
        df[f"{alg}_over_opt"] = df[alg] / df["opt"]
    return df, ref_is_opt


def total_mask(df, max_ratio):
    """k values at which the total bound is close enough to the dual to be shown."""
    if df["total_bound"].isna().all():
        return set(), "n/a for this objective"
    g = df.groupby("k")[["total_bound", "dual_bound"]].mean()
    keep = set(g.index[g["total_bound"] <= max_ratio * g["dual_bound"]])
    note = f"shown where <= {max_ratio:g}x dual" if keep else f"omitted: > {max_ratio:g}x dual at every k"
    return keep, note


def band(ax, df, col, color, label, keep=None, **kw):
    d = df if keep is None else df[df["k"].isin(keep)]
    if d.empty or d[col].isna().all():
        return
    g = d.groupby("k")[col].agg(["mean", "std"]).reset_index()
    ax.plot(g["k"], g["mean"], color=color, label=label, marker="o", **kw)
    ax.fill_between(g["k"], g["mean"] - g["std"].fillna(0), g["mean"] + g["std"].fillna(0),
                    color=color, alpha=0.15, linewidth=0)


def greedy_stop(df):
    """Smallest k at which plain greedy stopped before using its budget (non-monotone region)."""
    stopped = df[df["chain_len"] < df["k"] + 1]
    return int(stopped["chain_len"].min() - 1) if not stopped.empty else None


def instance_figure(df, ref_is_opt, path, max_ratio):
    problem, inst = df["problem"].iloc[0], df["instance"].iloc[0]
    n, seeds = int(df["n"].iloc[0]), df["seed"].nunique()
    stride = int(df["chain_stride"].max())
    ref = "OPT" if ref_is_opt else "best found"
    keep, note = total_mask(df, max_ratio)
    t = greedy_stop(df)

    fig, axes = plt.subplots(1, 3, figsize=(16, 4.4))
    ax = axes[0]
    for b, (label, color) in BOUNDS.items():
        band(ax, df, f"{b}_r", color, label if b != "total_bound" else f"{label} ({note})",
             keep=keep if b == "total_bound" else None)
    ax.axhline(1, color=OPT_COLOR, linestyle="--", linewidth=1, label=ref)
    if not keep:
        ax.text(0.02, 0.98, f"total-weight bound {note}", transform=ax.transAxes, va="top",
                fontsize=8, color="#52514e")
    ax.set_ylabel(f"upper bound / {ref}  (lower is tighter)")
    ax.set_title(f"Upper bounds relative to {ref}")
    hi = np.nanmax([df.groupby("k")[f"{b}_r"].mean().max() for b in ("dual_bound", "top_k_bound")])
    if hi > 4:
        ax.set_yscale("log")
        nice = [1, 1.5, 2, 3, 5, 7, 10, 15, 20, 30, 50, 100, 200, 500, 1000]
        ax.yaxis.set_major_locator(FixedLocator(nice))
        ax.yaxis.set_major_formatter(ScalarFormatter())
        ax.yaxis.set_minor_locator(NullLocator())
        ax.yaxis.set_minor_formatter(NullFormatter())

    for ax, alg, title in ((axes[1], "greedy", "Plain greedy"), (axes[2], "rg_mean", "Random greedy (mean)")):
        for b, (label, color) in BOUNDS.items():
            band(ax, df, f"{alg}_over_{b}", color, f"ALG / {label}", keep=keep if b == "total_bound" else None)
        if df["opt"].notna().any():
            band(ax, df, f"{alg}_over_opt", OPT_COLOR, "ALG / OPT (true)", linestyle="--")
        ax.axhline(INV_E, color=REF_COLOR, linestyle=":", linewidth=1.5, label="1/e guarantee")
        ax.set_ylim(0, 1.05)
        ax.set_ylabel("certified approximation ratio")
        ax.set_title(f"{title}: ALG / upper bound")

    for ax in axes:
        ax.set_xlabel("cardinality k")
        ax.xaxis.set_major_locator(MaxNLocator(integer=True))
        if t is not None:
            ax.axvline(t, color=REF_COLOR, linewidth=1, linestyle="-.")
    if t is not None:
        axes[0].text(t, 0.98, f" greedy stops\n at |S|={t}", transform=axes[0].get_xaxis_transform(),
                     va="top", fontsize=8, color="#52514e")

    h0, l0 = axes[0].get_legend_handles_labels()
    h1, l1 = axes[2].get_legend_handles_labels()
    fig.legend(h0 + h1, l0 + l1, loc="upper center", bbox_to_anchor=(0.5, 0.0), ncol=5, fontsize=8.5)
    sub = f"n={n}, {seeds} seed{'s' * (seeds > 1)}, mean ± 1 sd"
    if stride > 1:
        sub += f", dual over every {stride}th greedy prefix"
    fig.suptitle(f"{PROBLEM_NAMES.get(problem, problem)} — {inst}  ({sub})", fontsize=11)
    fig.tight_layout()
    for ext in ("png", "pdf"):
        fig.savefig(f"{path}.{ext}", bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {path}.png/.pdf")


def summary_row(df, ref_is_opt, max_ratio, label):
    keep, _ = total_mask(df, max_ratio)
    g = df.groupby("k")[["dual_bound", "top_k_bound", "total_bound"]].mean()
    others = g[["top_k_bound", "total_bound"]].min(axis=1, skipna=True)
    row = {
        "problem": PROBLEM_NAMES.get(df["problem"].iloc[0], df["problem"].iloc[0]),
        "instance": label, "n": int(df["n"].iloc[0]), "seeds": df["seed"].nunique(),
        "k": f"{df['k'].min()}–{df['k'].max()}", "ref": "OPT" if ref_is_opt else "best",
        "dual/ref": df["dual_bound_r"].mean(), "top-k/ref": df["top_k_bound_r"].mean(),
        "total/ref": df["total_bound_r"].mean(),
        "dual tightest": (g["dual_bound"] <= others + 1e-9).mean(),
        "min greedy/dual": df["greedy_over_dual_bound"].min(),
        "min rg/dual": df["rg_mean_over_dual_bound"].min(),
        "rg/dual ≥ 1/e": (df["rg_mean_over_dual_bound"] >= INV_E).mean(),
        "rg/top-k ≥ 1/e": (df["rg_mean_over_top_k_bound"] >= INV_E).mean(),
        "dual valid": bool((df["dual_valid"] == 1).all()),
    }
    return row


def overview_figure(frames, path):
    """greedy / dual vs k / n for every instance, one panel per problem."""
    problems = list(dict.fromkeys(df["problem"].iloc[0] for df, _ in frames))
    cols = 3
    rows = math.ceil(len(problems) / cols)
    fig, axes = plt.subplots(rows, cols, figsize=(5.2 * cols, 3.9 * rows), squeeze=False, sharey=True)
    for ax, prob in zip(axes.flat, problems):
        insts = [(df, r) for df, r in frames if df["problem"].iloc[0] == prob]
        shades = plt.cm.Blues(np.linspace(0.45, 0.95, len(insts)))
        for (df, _), c in zip(insts, shades):
            d = df.assign(kn=df["k"] / df["n"])
            g = d.groupby("kn")["greedy_over_dual_bound"].mean()
            ax.plot(g.index, g.values, marker="o", markersize=3, color=c,
                    label=f"{df['instance'].iloc[0]} (n={df['n'].iloc[0]})")
        ax.axhline(INV_E, color=REF_COLOR, linestyle=":", linewidth=1.5)
        ax.set_title(PROBLEM_NAMES.get(prob, prob))
        ax.set_xlabel("k / n")
        ax.set_ylim(0, 1.05)
        ax.legend(fontsize=7, loc="lower left")
    for ax in axes[:, 0]:
        ax.set_ylabel("greedy / dual bound")
    for ax in list(axes.flat)[len(problems):]:
        ax.set_visible(False)
    fig.suptitle("Certified ratio of plain greedy from the dual bound (dotted: 1/e)", fontsize=11)
    fig.tight_layout()
    for ext in ("png", "pdf"):
        fig.savefig(f"{path}.{ext}", bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {path}.png/.pdf")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results", nargs="+", default=["results"], help="directories with instance CSVs")
    ap.add_argument("--out", default="figures", help="output directory (per-instance figures go to "
                    "<results>/../figures when several result directories are given)")
    ap.add_argument("--total-max-ratio", type=float, default=1.5)
    ap.add_argument("--summary", help="write a markdown summary table to this file")
    args = ap.parse_args()

    frames, rows = [], []
    for res in args.results:
        out = args.out if len(args.results) == 1 else os.path.join(os.path.dirname(os.path.abspath(res)), "figures")
        os.makedirs(out, exist_ok=True)
        for f in sorted(glob.glob(os.path.join(res, "*.csv"))):
            if f.endswith("_prefix.csv") or f.endswith("_baselines.csv"):
                continue
            df, ref_is_opt = load(f)
            if df.empty:
                continue
            label = os.path.splitext(os.path.basename(f))[0]
            instance_figure(df, ref_is_opt, os.path.join(out, f"{label}_k_sweep"), args.total_max_ratio)
            frames.append((df, ref_is_opt))
            rows.append(summary_row(df, ref_is_opt, args.total_max_ratio, label))

    if not rows:
        print("no result CSVs found")
        return
    table = pd.DataFrame(rows)
    pct = ["dual tightest", "rg/dual ≥ 1/e", "rg/top-k ≥ 1/e"]
    fmt = table.copy()
    for c in pct:
        fmt[c] = fmt[c].map(lambda v: f"{v:.0%}")
    for c in ["dual/ref", "top-k/ref", "total/ref", "min greedy/dual", "min rg/dual"]:
        fmt[c] = fmt[c].map(lambda v: "n/a" if pd.isna(v) else f"{v:.3f}")
    print()
    print(fmt.to_string(index=False))

    if len(args.results) > 1:
        os.makedirs(args.out, exist_ok=True)
        overview_figure(frames, os.path.join(args.out, "overview_greedy_over_dual"))
    if args.summary:
        with open(args.summary, "w") as fh:
            fh.write("| " + " | ".join(fmt.columns) + " |\n")
            fh.write("|" + "---|" * len(fmt.columns) + "\n")
            for _, r in fmt.iterrows():
                fh.write("| " + " | ".join(str(v) for v in r.values) + " |\n")
        print(f"\nwrote {args.summary}")


if __name__ == "__main__":
    main()

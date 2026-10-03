"""Plots for the max-cut dual-bound experiments (run_*_sweep.sh -> results/*.csv).

Compares the dual upper bound against the two baseline upper bounds (sum of top-k
degrees, total edge weight) and against the 1/e guarantee of Random Greedy.

For an upper bound B >= OPT, ALG / B is a *certified* approximation ratio: it is
computable without OPT and is always <= ALG / OPT. A tighter bound gives a certified
ratio closer to the true one; whenever ALG / B >= 1/e the instance-specific
certificate beats the worst-case guarantee.

Usage:
    python plot_maxcut_results.py                      # all results/*.csv -> figures/
    python plot_maxcut_results.py --k-csv maxcut_results_n20_p0.3.csv
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
from matplotlib.ticker import MaxNLocator

INV_E = 1 / math.e

# bound -> (column suffix, label, color); color follows the bound in every figure
BOUNDS = {
    "dual": ("dual", "Dual bound", "#2a78d6"),
    "top_k": ("top_k", "Top-k degrees", "#eb6834"),
    "total": ("total", "Total edge weight", "#1baf7a"),
}
OPT_COLOR = "#0b0b0b"
REF_COLOR = "#8a8984"
ALGS = {"rg": "Random greedy (mean)", "greedy": "Plain greedy"}

plt.rcParams.update({
    "figure.dpi": 130,
    "savefig.dpi": 200,
    "font.size": 10,
    "axes.titlesize": 11,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.grid": True,
    "grid.color": "#e4e3df",
    "grid.linewidth": 0.6,
    "axes.edgecolor": "#8a8984",
    "lines.linewidth": 2,
    "lines.markersize": 4.5,
    "legend.frameon": False,
})


def save(fig, out_dir, name):
    for ext in ("png", "pdf"):
        fig.savefig(os.path.join(out_dir, f"{name}.{ext}"), bbox_inches="tight")
    plt.close(fig)
    print(f"  wrote {out_dir}/{name}.png/.pdf")


def band(ax, df, x, col, color, label, **kw):
    """Mean over seeds with a +-1 std band."""
    g = df.groupby(x)[col].agg(["mean", "std"]).reset_index()
    ax.plot(g[x], g["mean"], color=color, label=label, marker="o", **kw)
    ax.fill_between(g[x], g["mean"] - g["std"].fillna(0), g["mean"] + g["std"].fillna(0),
                    color=color, alpha=0.15, linewidth=0)


def legend_below(fig, ax, ncol=5):
    """One shared legend under the whole figure, so it never covers data."""
    handles, labels = ax.get_legend_handles_labels()
    fig.legend(handles, labels, loc="upper center", bbox_to_anchor=(0.5, 0.0),
               ncol=ncol, fontsize=8.5)


def one_over_e_line(ax):
    ax.axhline(INV_E, color=REF_COLOR, linestyle=":", linewidth=1.5, label="1/e guarantee")


def has_opt(df):
    return df["opt"].notna().any()


def reference(df):
    """Denominator for bound tightness: OPT when known, else the best solution found
    (max of plain greedy and the best random-greedy run). Since best_found <= OPT,
    bound / best_found over-estimates bound / OPT, so it is a conservative proxy."""
    if df["opt"].notna().all():
        return df["opt"], "OPT"
    return df[["greedy", "rg_max"]].max(axis=1), "best found"


# --------------------------------------------------------------------- k sweep
def plot_bound_tightness(df, x, xlabel, title, ax):
    ref, ref_name = reference(df)
    cols = {"dual": "dual_bound", "top_k": "top_k_bound", "total": "total_weight"}
    df = df.assign(**{f"_{key}_ratio": df[c] / ref for key, c in cols.items()})
    for key, (suf, label, color) in BOUNDS.items():
        band(ax, df, x, f"_{key}_ratio", color, label)
    ax.axhline(1, color=OPT_COLOR, linewidth=1, linestyle="--", label=ref_name)
    ax.set_xlabel(xlabel)
    if x == "k":
        ax.xaxis.set_major_locator(MaxNLocator(integer=True))
    ax.set_ylabel(f"upper bound / {ref_name}  (lower is tighter)")
    if max(df[f"_{key}_ratio"].groupby(df[x]).mean().max() for key in cols) > 5:
        ax.set_yscale("log")  # total weight can be orders of magnitude off for small k
    ax.set_title(title)


def plot_certified(df, x, xlabel, alg, ax, show_true=True):
    for key, (suf, label, color) in BOUNDS.items():
        band(ax, df, x, f"{alg}_over_{suf}", color, f"{ALGS[alg].split(' (')[0]} / {label}")
    if show_true and has_opt(df):
        band(ax, df, x, f"{alg}_over_opt", OPT_COLOR, f"{ALGS[alg].split(' (')[0]} / OPT (true)",
             linestyle="--")
    one_over_e_line(ax)
    ax.set_xlabel(xlabel)
    if x == "k":
        ax.xaxis.set_major_locator(MaxNLocator(integer=True))
    ax.set_ylabel("certified approximation ratio")
    ax.set_ylim(0, 1.05)
    ax.set_title(ALGS[alg])


def k_sweep_figures(df, tag, out_dir):
    n, p = df["n"].iloc[0], df["p"].iloc[0]
    seeds = df["seed"].nunique()
    sub = f"G(n={n}, p={p:g}), {seeds} graph seed{'s' * (seeds > 1)}, mean ± 1 sd"

    ref, ref_name = reference(df)
    fig, ax = plt.subplots(figsize=(6.4, 4.2))
    plot_bound_tightness(df, "k", "cardinality k", f"Upper bounds relative to {ref_name}\n{sub}", ax)
    ax.legend()
    save(fig, out_dir, f"{tag}_bound_tightness")

    fig, axes = plt.subplots(1, 2, figsize=(11.5, 4.2), sharey=True)
    for ax, alg in zip(axes, ALGS):
        plot_certified(df, "k", "cardinality k", alg, ax)
    for ax in axes:
        ax.legend(loc="upper center", bbox_to_anchor=(0.5, -0.16), ncol=2, fontsize=8.5)
    fig.suptitle(f"Certified ratio ALG / upper bound vs. 1/e — {sub}", fontsize=11)
    save(fig, out_dir, f"{tag}_certified_ratio")

    # per-run scatter: is the dual bound ever looser than top-k?
    fig, ax = plt.subplots(figsize=(4.8, 4.6))
    df = df.assign(_x=df["top_k_bound"] / ref, _y=df["dual_bound"] / ref)
    xcol, ycol, unit = "_x", "_y", f" / {ref_name}"
    blues = matplotlib.colors.ListedColormap(plt.cm.Blues(np.linspace(0.3, 1.0, 256)))
    sc = ax.scatter(df[xcol], df[ycol], c=df["k"], cmap=blues, s=22,
                    edgecolors="#fcfcfb", linewidths=0.6)
    lim = [min(df[xcol].min(), df[ycol].min()), max(df[xcol].max(), df[ycol].max())]
    ax.plot(lim, lim, color=REF_COLOR, linestyle="--", linewidth=1, label="equal")
    ax.set_xlabel(f"top-k bound{unit}")
    ax.set_ylabel(f"dual bound{unit}")
    frac = (df["dual_bound"] < df["top_k_bound"] - 1e-9).mean()
    ax.set_title(f"Per-run comparison ({len(df)} runs)\ndual strictly tighter in {frac:.0%}")
    fig.colorbar(sc, ax=ax, label="k")
    ax.legend(loc="upper left")
    save(fig, out_dir, f"{tag}_dual_vs_topk_scatter")


def prefix_figure(pf, tag, out_dir, ks=None):
    """Which greedy prefix S_i attains min_i f(S_i)+penalty(S_i)+dual(S_i)?"""
    all_ks = sorted(pf["k"].unique())
    ks = ks or [all_ks[int(round(q * (len(all_ks) - 1)))] for q in (0.1, 0.25, 0.45, 0.7, 0.9)]
    ks = sorted(set(ks))
    norm = pf["opt"].notna().all()
    if norm:
        y, ref_name = pf["bound_S"] / pf["opt"], "OPT"
    else:  # f of the last prefix is the plain-greedy value
        last = pf.groupby(["n", "p", "k", "seed"])["prefix"].transform("max") == pf["prefix"]
        greedy = pf[last].set_index(["n", "p", "k", "seed"])["f_S"]
        y = pf["bound_S"].values / greedy.reindex(pd.MultiIndex.from_frame(pf[["n", "p", "k", "seed"]])).values
        ref_name = "greedy value"
    pf = pf.assign(y=y)
    fig, axes = plt.subplots(1, 2, figsize=(11.5, 4.2))
    shades = plt.cm.Blues(np.linspace(0.45, 0.95, len(ks)))
    for k, c in zip(ks, shades):
        g = pf[pf["k"] == k].groupby("prefix")["y"].mean()
        axes[0].plot(g.index, g.values, marker="o", color=c, label=f"k={k}")
    axes[0].axhline(1, color=OPT_COLOR, linewidth=1, linestyle="--", label=ref_name)
    axes[0].set_xlabel("greedy prefix index i  (|S_i| = i)")
    axes[0].set_ylabel(f"f(S_i) + penalty(S_i) + dual(S_i)  / {ref_name}")
    axes[0].set_title("Bound obtained from each greedy prefix (mean over seeds)")
    axes[0].legend(fontsize=8.5)

    best = pf.loc[pf.groupby(["n", "p", "k", "seed"])["bound_S"].idxmin()]
    counts = best.groupby("k")["prefix"].apply(lambda s: (s == 0).mean())
    step = np.diff(all_ks).min() if len(all_ks) > 1 else 1
    axes[1].bar(counts.index, 1 - counts.values, color=BOUNDS["dual"][2], width=0.7 * step)
    axes[1].set_ylim(0, 1)
    axes[1].set_xlabel("cardinality k")
    axes[1].xaxis.set_major_locator(MaxNLocator(integer=True))
    axes[1].set_ylabel("fraction of runs")
    axes[1].set_title("Runs where a non-empty prefix (i > 0) gives the min")
    if (best["prefix"] == 0).all():
        axes[1].text(0.5, 0.5, f"none: S_0 = {{}} gave the min in all {len(best)} runs",
                     transform=axes[1].transAxes, ha="center", va="center", color="#52514e")
    save(fig, out_dir, f"{tag}_prefix")


def k_sweep_overview(dfs, out_dir, name="k_sweep_overview"):
    """Several k sweeps with different n on a common x = k / n axis."""
    df = pd.concat(dfs, ignore_index=True)
    ref, ref_name = reference(df)
    df = df.assign(k_frac=df["k"] / df["n"], _dual=df["dual_bound"] / ref,
                   _topk=df["top_k_bound"] / ref, _total=df["total_weight"] / ref)
    ns = sorted(df["n"].unique())
    shades = plt.cm.Blues(np.linspace(0.45, 0.95, len(ns)))
    panels = [("rg_over_dual", "Random greedy / dual bound", "certified approximation ratio"),
              ("_dual", f"Dual bound / {ref_name}", f"bound / {ref_name}"),
              ("_topk", f"Top-k bound / {ref_name}", f"bound / {ref_name}"),
              ("_total", f"Total edge weight / {ref_name}", f"bound / {ref_name}")]
    fig, axes = plt.subplots(1, 4, figsize=(17, 4.2))
    for ax, (col, title, ylabel) in zip(axes, panels):
        for n, c in zip(ns, shades):
            g = df[df["n"] == n].groupby("k_frac")[col].mean()
            ax.plot(g.index, g.values, marker="o", color=c, label=f"n={n}")
        ax.set_title(title)
        ax.set_xlabel("k / n")
        ax.set_ylabel(ylabel)
    one_over_e_line(axes[0])
    axes[0].set_ylim(0, 1.05)
    hi = max(df[c].groupby([df["n"], df["k_frac"]]).mean().max() for c in ("_dual", "_topk"))
    for ax in axes[1:3]:
        ax.set_ylim(0.95, hi * 1.05)
    for ax in axes[1:]:
        ax.axhline(1, color=OPT_COLOR, linewidth=1, linestyle="--")
    axes[3].set_yscale("log")
    p = df["p"].iloc[0]
    fig.suptitle(f"k sweeps across graph sizes (p={p:g}, mean over seeds)", fontsize=11)
    fig.tight_layout()
    legend_below(fig, axes[0], ncol=len(ns) + 1)
    save(fig, out_dir, name)


# --------------------------------------------------------------- density sweep
def density_figures(df, tag, out_dir):
    ks = sorted(df["k"].unique())
    n = df["n"].iloc[0]
    rows = 2 if has_opt(df) else 1
    fig, axes = plt.subplots(rows, len(ks), figsize=(4.2 * len(ks), 3.8 * rows),
                             sharex=True, squeeze=False)
    for j, k in enumerate(ks):
        d = df[df["k"] == k]
        plot_certified(d, "p", "edge probability p", "rg", axes[0, j])
        axes[0, j].set_title(f"Random greedy, k={k}")
        if rows == 2:
            plot_bound_tightness(d, "p", "edge probability p", f"Bounds / OPT, k={k}", axes[1, j])
            axes[0, j].set_xlabel("")
    fig.suptitle(f"Effect of density, n={n} (mean ± 1 sd over seeds)", fontsize=11)
    fig.tight_layout()
    legend_below(fig, axes[0, 0])
    save(fig, out_dir, f"{tag}_density")


# ------------------------------------------------------------------ size sweep
def size_figures(df, tag, out_dir):
    ks = sorted(df["k"].unique())
    p = df["p"].iloc[0]
    fig, axes = plt.subplots(1, len(ks), figsize=(3.9 * len(ks), 3.9), sharey=True, squeeze=False)
    for ax, k in zip(axes[0], ks):
        d = df[df["k"] == k]
        plot_certified(d, "n", "number of vertices n", "rg", ax, show_true=False)
        ax.set_xscale("log", base=2)
        ax.set_xticks(sorted(d["n"].unique()), [str(v) for v in sorted(d["n"].unique())])
        ax.set_title(f"k={k}")
    fig.suptitle(f"Certified ratio of Random greedy as n grows (p={p:g}; OPT unknown for n>20)",
                 fontsize=11)
    fig.tight_layout()
    legend_below(fig, axes[0, 0], ncol=4)
    save(fig, out_dir, f"{tag}_certified_ratio")

    fig, ax = plt.subplots(figsize=(5.6, 4.2))
    shades = plt.cm.Blues(np.linspace(0.45, 0.95, len(ks)))
    for k, c in zip(ks, shades):
        g = df[df["k"] == k].groupby("n")["time_dual_ms"].mean()
        ax.plot(g.index, g.values, marker="o", color=c, label=f"k={k}")
    ax.set_xscale("log", base=2)
    ns = sorted(df["n"].unique())
    ax.set_xticks(ns, [str(v) for v in ns])
    ax.set_yscale("log")
    ax.set_xlabel("number of vertices n")
    ax.set_ylabel("dual bound time (ms, mean over seeds)")
    ax.set_title("Cost of computing the dual bound")
    ax.legend()
    save(fig, out_dir, f"{tag}_dual_runtime")


# --------------------------------------------------------------------- summary
def summary(df, name):
    print(f"\n== {name}: {len(df)} runs ==")
    ref, ref_name = reference(df)
    cols = {"dual": "dual_bound", "top_k": "top_k_bound", "total": "total_weight"}
    rows = []
    for key, (suf, label, _) in BOUNDS.items():
        r = df[f"rg_over_{suf}"]
        rows.append({"bound": label,
                     f"mean bound/{ref_name}": (df[cols[key]] / ref).mean(),
                     "min rg/bound": r.min(), "mean rg/bound": r.mean(),
                     "certifies >= 1/e": f"{(r >= INV_E).mean():.1%}"})
    print(pd.DataFrame(rows).to_string(index=False, float_format=lambda v: f"{v:.3f}"))
    if has_opt(df):
        print(f"true rg/OPT: min {df['rg_over_opt'].min():.3f}, mean {df['rg_over_opt'].mean():.3f}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results", default="results", help="directory with the sweep CSVs")
    ap.add_argument("--out", default="figures", help="output directory for figures")
    ap.add_argument("--k-csv", nargs="*", help="k-sweep CSVs (default: results/k_sweep_*.csv)")
    ap.add_argument("--density-csv", nargs="*", help="default: results/density_sweep_*.csv")
    ap.add_argument("--size-csv", nargs="*", help="default: results/size_sweep_*.csv")
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)

    def find(given, pattern):
        if given is not None:
            return given
        return sorted(f for f in glob.glob(os.path.join(args.results, pattern))
                      if not f.endswith("_prefix.csv"))

    def tag_of(path):
        return os.path.splitext(os.path.basename(path))[0]

    k_dfs = []
    for f in find(args.k_csv, "k_sweep_*.csv"):
        df = pd.read_csv(f)
        k_dfs.append(df)
        print(f"{f}:")
        k_sweep_figures(df, tag_of(f), args.out)
        pfx = f[:-4] + "_prefix.csv"
        if os.path.exists(pfx):
            prefix_figure(pd.read_csv(pfx), tag_of(f), args.out)
        summary(df, tag_of(f))
    if len(k_dfs) > 1 and pd.concat(k_dfs)["n"].nunique() > 1:
        k_sweep_overview(k_dfs, args.out)
    for f in find(args.density_csv, "density_sweep_*.csv"):
        df = pd.read_csv(f)
        print(f"{f}:")
        density_figures(df, tag_of(f), args.out)
        summary(df, tag_of(f))
    for f in find(args.size_csv, "size_sweep_*.csv"):
        df = pd.read_csv(f)
        print(f"{f}:")
        size_figures(df, tag_of(f), args.out)
        summary(df, tag_of(f))


if __name__ == "__main__":
    main()

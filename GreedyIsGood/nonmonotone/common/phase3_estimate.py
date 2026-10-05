"""Phase-3 runtime estimate: untruncated base sets (every greedy prefix at every k).

Today each k uses the greedy prefixes S_0..S_min(k,t) with t = min(k_max, t_full), where t_full is the
length of the unbudgeted greedy chain (greedy stops once no gain is positive). The DP rows of every
prefix already cover all k <= k_max, so using *all* computed prefixes at every k is free; the only new
NM-Dual work is the prefixes t+1..t_full (respecting the instance's stride), each costing one
high_cap_U call. This script measures t_full (GREEDY_LENGTH_ONLY=1 run of the problem binary, seed 0),
takes the per-prefix NM-Dual costs from <problem>/results/logs/<label>_baselines.log (seed 0 job), and
estimates:
  * extra NM-Dual time = (#new prefixes) x (mean cost of the last 5 measured prefixes)  [an over-estimate:
    later prefixes have fewer free elements]; the --baselines run computes the caps a second time for B5;
  * the B2/B5 LPs at every k would then contain every base set, i.e. the size they now have only at k_max
    (or larger when t_full > k_max).

Usage: python common/phase3_estimate.py [--out phase3_estimate.md]     (run from GreedyIsGood/nonmonotone)
"""
import argparse
import csv
import os
import re
import subprocess

ML = "../../../Coverage/data/ML/ml-1m/ratings.dat"
WINE = "--features data/winequality-red.csv --drop-last"
# (problem dir, binary, label, args for seed 0) -- mirrors the run_k_sweep.sh scripts
JOBS = [
    ("src", "./maxcut_baselines", "maxcut_generic", "--n 20 --p 0.3 --ks 1:20"),
    ("directed_cut", "./directed_cut", "synthetic_n20", "--synthetic 20 --p 0.3 --ks 1:20"),
    ("directed_cut", "./directed_cut", "synthetic_n200", "--synthetic 200 --p 0.1 --ks 1,5:100:5"),
    ("directed_cut", "./directed_cut", "email_eu_core", "--graph data/email-Eu-core.txt --ks 1,20:500:20"),
    ("directed_cut", "./directed_cut", "wiki_vote", "--graph data/wiki-Vote.txt --ks 1,10:200:10 --chain-stride 10"),
    ("revenue_max", "./revenue_max", "synthetic_n20", "--synthetic 20 --p 0.3 --ks 1:20"),
    ("revenue_max", "./revenue_max", "synthetic_n200", "--synthetic 200 --p 0.1 --ks 1,5:100:5"),
    ("revenue_max", "./revenue_max", "facebook", "--graph data/facebook_combined.txt --ks 1,10:200:10 --chain-stride 10"),
    ("revenue_max", "./revenue_max", "youtube", "--graph data/youtube_cmty_3000.txt --ks 1,10:200:10 --chain-stride 5"),
    ("diverse_recommendation", "./diverse_rec", "ml20_lambda0.75", f"--movielens {ML} --top 200 --sample 20 --lambda 0.75 --ks 1:20"),
    ("diverse_recommendation", "./diverse_rec", "ml20_lambda1.0", f"--movielens {ML} --top 200 --sample 20 --lambda 1.0 --ks 1:20"),
    ("diverse_recommendation", "./diverse_rec", "ml500_lambda0.75", f"--movielens {ML} --top 1000 --sample 500 --lambda 0.75 --ks 1,10:250:10"),
    ("diverse_recommendation", "./diverse_rec", "ml500_lambda1.0", f"--movielens {ML} --top 1000 --sample 500 --lambda 1.0 --ks 1,10:250:10"),
    ("log_determinant", "./log_det", "synthetic_n20_alpha3", "--synthetic 20 --alpha 3 --ks 1:20"),
    ("log_determinant", "./log_det", "wine_n100_alpha3", f"{WINE} --sample 100 --alpha 3 --ks 1:50"),
    ("log_determinant", "./log_det", "wine_n100_alpha10", f"{WINE} --sample 100 --alpha 10 --ks 1:50"),
    ("log_determinant", "./log_det", "wine_n300_alpha3", f"{WINE} --sample 300 --alpha 3 --ks 1,5:100:5 --chain-stride 5"),
    ("log_determinant", "./log_det", "wine_n300_alpha10", f"{WINE} --sample 300 --alpha 10 --ks 1,5:100:5 --chain-stride 5"),
    ("gaussian_mi", "./gaussian_mi", "synthetic_gp_n20", "--synthetic 20 --ks 1:20"),
    ("gaussian_mi", "./gaussian_mi", "synthetic_gp_n100", "--synthetic 100 --ks 1:50"),
    ("gaussian_mi", "./gaussian_mi", "intel", "--cov data/intel_temp_cov.txt --ks 1:52"),
    ("hypergraph_cut", "./hypergraph_cut", "synthetic_n20", "--synthetic 20 --edges 60 --ks 1:20"),
    ("hypergraph_cut", "./hypergraph_cut", "synthetic_n500", "--synthetic 500 --edges 2000 --ks 1,10:250:10"),
    ("hypergraph_cut", "./hypergraph_cut", "mag10_top1000", "--hypergraph data/mag10_top1000.txt --ks 1,10:300:10"),
]


def prefix_costs(log):
    """(index, ms) of the prefixes of the first job in the log (seed 0)"""
    out, started = [], False
    for line in open(log):
        if line.startswith("["):
            if started:
                break
            started = True
        m = re.match(r"\s+prefix (\d+)/(\d+):.*\((\d+(?:\.\d+)?) ms\)", line)
        if m:
            out.append((int(m.group(1)), float(m.group(3))))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="phase3_estimate.md")
    args = ap.parse_args()
    rows = []
    for folder, binary, label, jargs in JOBS:
        r = subprocess.run(f"{binary} {jargs} --seed 0 --trials 0 --out /dev/null", shell=True, cwd=folder,
                           env={**os.environ, "GREEDY_LENGTH_ONLY": "1"}, capture_output=True, text=True)
        m = re.search(r"GREEDY_LENGTH (\d+)", r.stdout)
        if not m:
            raise SystemExit(f"{folder}/{label}: no greedy length\n{r.stdout}\n{r.stderr}")
        t_full = int(m.group(1))
        with open(os.path.join(folder, "results", f"{label}_baselines.csv")) as f:
            recs = [x for x in csv.DictReader(f) if x["seed"] == "0"]
        n, kmax = int(recs[0]["n"]), max(int(x["k"]) for x in recs)
        stride = int(recs[0].get("chain_stride", 1) or 1)
        costs = prefix_costs(os.path.join(folder, "results", "logs", f"{label}_baselines.log"))
        t = max(i for i, _ in costs)
        new = [i for i in range(t + 1, t_full + 1) if i % stride == 0 or i == t_full]
        per = sum(c for _, c in costs[-5:]) / len(costs[-5:])
        dual_now = sum(c for _, c in costs) / 1000
        sets_now = len(costs)
        cols_now = max(int(x.get("hybrid_cols", 0) or 0) for x in recs)
        add_cols = sum(n - i for i in new)
        hy_now = sum(float(x.get("time_hybrid_solve_ms", 0) or 0) for x in recs) / 1000
        max_lp = max(float(x.get("time_hybrid_solve_ms", 0) or 0) for x in recs) / 1000
        nk = len(recs)
        hy_new = nk * max_lp * (cols_now + add_cols) / max(cols_now, 1)
        rows.append([folder, label, n, kmax, stride, t, t_full, len(new), f"{dual_now:.1f}",
                     f"{len(new) * per / 1000:.1f}", f"{sets_now} -> {sets_now + len(new)}",
                     f"{cols_now} -> {cols_now + add_cols}", f"{hy_now:.1f}", f"{hy_new:.1f}"])
        print(f"{folder}/{label}: t={t} t_full={t_full} new prefixes={len(new)}", flush=True)
    head = ["problem", "instance", "n", "k_max", "stride", "t (prefixes now)", "t_full", "new prefixes",
            "NM-Dual now [s]", "extra NM-Dual [s]", "base sets at every k", "max hybrid cols",
            "hybrid solve now, all k [s]", "hybrid solve est., all k [s]"]
    with open(args.out, "w") as f:
        f.write("# Phase 3 estimate: untruncated base sets (seed 0 of each instance)\n\n")
        f.write("Extra NM-Dual time = new prefixes x mean cost of the last 5 measured prefixes (an over-estimate). "
                "The --baselines run recomputes the caps for B5, so its extra time is about twice this. Hybrid solve "
                "estimate = #k x (largest current LP time) x (new columns / current columns), assuming linear scaling; "
                "B2 grows the same way but its solves are far cheaper.\n\n")
        f.write("| " + " | ".join(head) + " |\n|" + "---|" * len(head) + "\n")
        for r in rows:
            f.write("| " + " | ".join(map(str, r)) + " |\n")
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()

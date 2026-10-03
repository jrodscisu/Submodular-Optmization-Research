# Dual upper bounds for non-monotone submodular maximization under |S| ≤ k

Experiments comparing the **dual upper bound** (`penalty_S` + `high_cap_U` + DP, minimized over
the plain-greedy chain, as in `src/maxcut_random_greedy.cpp`) against the **top-k singleton**
bound and a problem-specific **total-weight** bound, and the certified ratio ALG / bound of plain
greedy and random greedy against the 1/e guarantee.

```
nonmonotone/
├── src/                      max-cut (original code, sweeps and plots)
├── common/
│   ├── dual_core.hpp         problem-independent port of the max-cut algorithms
│   ├── experiment.hpp        k-sweep driver + CSV output + self-checks
│   ├── inverse_set.hpp       incremental K_A^{-1} for log-det and Gaussian MI
│   ├── graph_io.hpp          edge lists, G(n,p) generator
│   ├── run_jobs.sh           parallel job runner used by every run_k_sweep.sh
│   ├── plot_k_sweep.py       per-instance figures, overview figure, summary table
│   ├── validate.sh           correctness checks on small instances (see below)
│   ├── port_check.cpp        generic port vs. the original max-cut binary
│   └── build_all.sh
├── <problem>/                one folder per problem:
│   ├── <problem>.cpp         objective, incremental gain oracle, instance loaders
│   ├── run_k_sweep.sh        the k-sweep experiments for this problem
│   ├── data/download.sh      fetches + prepares the datasets (not committed)
│   ├── results/              <instance>.csv, <instance>_prefix.csv, logs/
│   └── figures/              <instance>_k_sweep.png/.pdf
├── figures/                  cross-problem overview
├── RESULTS_k_sweep.md        summary table over all instances
└── run_all_problems.sh
```

| Problem | f(S) | Datasets | top-k bound | total bound |
|---|---|---|---|---|
| `directed_cut` | weight of edges leaving S | G(n,p) with random orientations; SNAP email-Eu-core, wiki-Vote | k largest out-degrees | Σ w |
| `revenue_max` | Σ_{i∉S} √(Σ_{j∈S} w_ij) | G(n,p); SNAP ego-Facebook; com-YouTube community subgraph (U(0,1) weights) | k largest f({v}) | Σ_i √(Σ_j w_ij) |
| `diverse_recommendation` | Σ_{i∈S}Σ_j s_ij − λ Σ_{i,j∈S} s_ij | MovieLens 1M (cosine similarity of rating vectors), λ ∈ {0.75, 1} | k largest f({v}) | Σ_ij s_ij |
| `log_determinant` | log det L_S, L = α(K_RBF + 10⁻³ I) | synthetic mixture; UCI wine-quality-red, α ∈ {3, 10} | k largest log L_vv | n/a |
| `gaussian_mi` | I(X_S; X_{V∖S}) | GP on random points; Intel Berkeley lab temperatures (52 motes) | k largest f({v}) | n/a |
| `hypergraph_cut` | weight of hyperedges split by S | random hypergraphs; co-authorship cat-edge-MAG-10 (top 1000 authors) | k largest weighted degrees | Σ w_e |

Every problem has a small instance (n ≤ 20, 10 seeds) where OPT is brute-forced for all k, plus
larger synthetic and real instances where the reference is the best solution found.

## Running

```bash
common/validate.sh                 # ~15 s: oracle, sweep-vs-direct and dual >= OPT checks
<problem>/run_k_sweep.sh           # one problem (optionally: instance names)
./run_all_problems.sh              # everything + overview figure + RESULTS_k_sweep.md
```
Plotting needs `pandas` and `matplotlib` (`PYTHON=/path/to/python` selects the interpreter).
`JOBS` sets the number of parallel processes. Each binary prints its options at the top of its
source file; the shared sweep options are `--ks 1,10:200:10 --trials --seed --out --prefix-out
--brute-budget --chain-stride --check-oracle --check-direct`.

## How the dual bound is computed for a k sweep

`common/dual_core.hpp` is the max-cut code with the graph replaced by a problem object exposing
`gain(v) = f(S+v) − f(S−v)`, `add`, `remove` and `eval`; `common/port_check.cpp` reproduces all
200 rows of `src/results/k_sweep_n20_p0.3.csv` exactly.

For a sweep over k, the plain-greedy prefixes S_0, S_1, … are the same for every budget and the
DP rows of `dual_upper_bound` do not depend on k, so each prefix is evaluated once and

    dual(k) = min_{i ≤ min(k, t)} f(S_i) + penalty(S_i) + max(0, max_{j≤k} dp_i[j]),

which equals `dual_wrapper(k, plain_greedy(k))`; `--check-direct` asserts it. `high_cap_U` costs
O(n²) oracle calls per prefix, so the largest instances use `--chain-stride s` (only every s-th
prefix, plus the last prefix of each k). A minimum over fewer sets is still a valid upper bound,
just possibly looser; figures say when a stride was used.

## Notes

* `log_determinant`: log det is not non-negative, so the 1/e guarantee of random greedy does not
  formally apply (OPT ≥ f(∅) = 0 still holds); the dual bound was still ≥ OPT on all small
  instances. There is no natural "total weight" bound for log-det or Gaussian MI.
* The total-weight bound is drawn only for k where it is within 1.5× of the dual bound
  (`--total-max-ratio`); otherwise it is orders of magnitude off and hides the other curves.
* `CSV` columns: `opt` (nan when not brute-forced), `top_k_bound`, `total_bound`, `dual_bound`,
  `dual_bound_S0` (bound from S_0 = ∅ alone), `dual_best_prefix`, `greedy`, `rg_mean/std/min/max`,
  `best_found`, `dual_valid` (dual ≥ best solution found), timings.

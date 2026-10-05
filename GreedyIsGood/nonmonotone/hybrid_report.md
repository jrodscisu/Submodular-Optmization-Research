# Hybrid LP (B5) = B2 + NM-Dual's prefix caps: ablation and checks

B5 adds NM-Dual's prefix caps to the unified LP B2, on exactly the same base sets (the plain-greedy
prefixes NM-Dual minimizes over at each k, stride included), instances, seeds and k grids as the
existing results. NM-Dual, the objectives, generators, seeds and the existing baselines are
unchanged; regenerating every earlier figure from the new run reproduces the committed files byte
for byte.

* **NM-Dual's caps (answers to the pre-coding questions).** `high_cap_U` uses *both* cap variants
  and takes the smaller: U_i = min(A-cap, X-cap), where the A-cap is
  f_S(A_i) + Σ_{j≤i}[−f(a_j | S∪A_i−a_j)]⁺ (prefix X = A_i) and the X-cap is
  f_S(X_i) + Σ_{x∈X_i}[−f(x | S∪A_i−x)]⁺ + Σ_{a∈A_i∖X_i}[f(a | S∪X_i)]⁺ with the incremental X_i;
  then Ũ_i = min_{j≥i} U_j. The caps cover **all** of V∖S (the ordering includes elements with
  f(a | S) < 0), so the m₊ fallback was not needed. Base sets at the largest k range from 6 (log-det
  wine n = 300, α = 3) to 303 (email-Eu-core); the largest B5 LP is email-Eu-core at k = 500 with
  259,768 columns and 259,066 rows (C2 imposed as variable upper bounds, which is equivalent and
  halves the rows; 517,828 rows if written as constraints), 3 nonzeros per C1 row.
* **Implementation.** `common/baselines.hpp`: `nm_dual_caps` recomputes each base set's ordering,
  marginals and caps with the same code path and the same `high_cap_U` as NM-Dual; the driver
  rebuilds NM-Dual's DP rows from them and aborts on any difference (none occurred), writes them to
  `results/lp_export/<instance>__seed<s>.hybrid.txt`, and, where brute force ran, an optimal set O per
  k with the certificate vectors P^S_i = f_S(O′ ∩ first i positions). `common/hybrid_lp.py` builds the
  cumulative-P LP (C0–C3, P free, x ∈ [0,1]) with scipy.sparse, solves it with HiGHS (8 LPs in
  parallel), stops on any non-optimal status, runs checks 1–3, and adds the Phase-2 `_mono`
  columns. Columns added to every `_baselines.csv`: `hybrid_bound, best_bound, hybrid_status,
  hybrid_rows, hybrid_cols, time_hybrid_export_ms, time_hybrid_solve_ms, hybrid_certificate,
  dual_bound_mono, lp_bound_mono, hybrid_bound_mono, best_bound_mono`.
* **Figures.** `latex/figures/<problem>_hybrid.tex` and `<problem>_wide_hybrid.tex` (demo
  `latex/main_hybrid.pdf`): all `_baselines` series plus RG/B5 (brown ⬟) and RG/min(dual, B5)
  (teal ⊗), same metric/axes/styles; the single-column version uses a 2-column legend to fit.
* **Runs.** `./run_all_baselines.sh` (each problem's `BASELINES=1 run_k_sweep.sh` now also runs
  `common/hybrid_lp.py`); tables below from `common/hybrid_report.py`.

## Summary

1. **Checks: 0 failures.** All 3,186 B5 LPs (one per row, max cut included) solved to optimality.
   Check 1 (B5 ≥ OPT_k) on all 1,684 rows with OPT; check 2 (B5 ≤ B2) on all 3,186 rows; check 3
   (the proof's certificate point satisfies C0–C3) on all 1,684 rows where brute force produced an
   optimal set — 0 violations each. Check 1 on the four `_mono` columns: 6,736 checks, 0 violations.
2. **The 2×2 ablation (k > 1).** Which single ingredient helps more depends on the instance: coupling
   the base sets through one x (B2) beats the caps (NM-Dual) on all n = 20 instances, on log-det and on
   email-Eu-core, YouTube, revenue G(200, 0.1) and MovieLens λ = 0.75 (n = 500), while the caps win
   on directed G(200, 0.1), Facebook, MovieLens λ = 1 (n = 500), Intel, GP n = 100, MAG-10 and
   hypergraph n = 500 (wiki-Vote ties). The two combine: B5 has the lowest mean bound of the four on
   every one of the 24 instances (ties with B2 on log-det), and min(NM-Dual, B5) is the tightest bound overall on
   19 of 24 instances (on the 5 log-det instances it equals B5/B2). On the n = 20 instances with OPT,
   mean bound/OPT drops from NM-Dual to min(NM-Dual, B5) as follows: max cut 1.48 → 1.28, directed
   cut 1.18 → 1.12, revenue 1.46 → 1.39, MovieLens λ = 0.75 1.53 → 1.41, λ = 1 2.02 → 1.85,
   log-det 2.44 → 1.90, GP 1.61 → 1.33, hypergraph cut 1.78 → 1.72.
3. **How much do the caps tighten the LP?** Little on average, more on cut-type objectives:
   (B2 − B5)/B2 has mean 1.8–2.2% (max 8.1%) on directed and hypergraph cut, 0.6–1.0% (max
   2.7–7.4%) on max cut, revenue, MovieLens and Gaussian MI, and ≈ 0 on log-det (max 0.9%). B5 is
   more than 1% below B2 on 56–63% of the directed/hypergraph rows, 25–36% for max cut, revenue,
   MovieLens and Gaussian MI, and 0% for log-det; per instance the largest share is Intel (90%) and
   MAG-10 (87%). So B5 mostly captures the coupling of B2; the caps add a consistent but modest
   improvement — in line with the Marginal-vs-NM-Dual finding that on log-det the caps are inactive.
4. **min(NM-Dual, B5) beats both NM-Dual alone and B2 alone** on 15–98% of the rows of every instance
   except synthetic log-det (0%, where B5 = B2 < NM-Dual): most often on the large instances
   (YouTube 98%, Facebook 90%, MovieLens n = 500 79–84%, hypergraph n = 500 71%, MAG-10 70%,
   wiki-Vote 70%). The gain over the better of the two is small: mean 0.0–1.9% on the winning rows,
   max 5.2% (directed cut n = 20). Per row, B5 is below NM-Dual in 70% of the 3,056 rows with k > 1
   and above it in 23% (by up to 10%), so the min is needed.
5. **Phase 2 (`_mono`) barely matters.** NM-Dual is already non-decreasing in k on every row (0 rows
   change). B2 and B5 are not always: since larger k uses more base sets, the LP at a larger k can be
   lower; the monotone fix changes ≤ 4% of rows (max cut, MovieLens), by up to 5.9% (B2) / 5.1%
   (B5), and on the other problems by at most 1.2% (Gaussian MI) on well under 1% of rows.
6. **Cost.** B5 is the expensive bound on the large instances: summed HiGHS time per seed over all k
   is 736 s on Facebook (35 s per LP), 267 s on hypergraph n = 500, 263 s on MAG-10, 215 s on
   MovieLens λ = 0.75 (n = 500), 193 s on email-Eu-core, 171 s on YouTube and 96 s on wiki-Vote,
   versus 1–85 s for NM-Dual and ≤ 1 s for B2. The export (recomputing the caps) costs about one
   NM-Dual run. These are solve times measured with 8 LPs in parallel. **Facebook exceeded the
   5-minute-per-instance limit** of the task (784 s for its slowest seed); the run had already
   completed when this was measured — the results are kept and nothing further was run.
7. **Corrections to `baselines_report.md`.** (a) The Marginal caveat now states that the bound is
   implemented (column `marginal_bound`, computed in `common/experiment.hpp --baselines` as
   min_S f(S) + Pen(S) + Σ_{top-k}[f(a|S)]⁺ over NM-Dual's base sets) instead of "nothing was done".
   (b) The "LP overtakes NM-Dual at k ≈ 0.2–0.5·n" claim is replaced by the recomputed per-instance
   crossover (new table "LP (B2) vs NM-Dual crossover per instance"): the first k ranges from
   k/n ≈ 0.005 (Facebook, YouTube) to 0.55 (MovieLens n = 20), the LP's region is not always a tail
   (Facebook k ∈ {20, 30}, YouTube 20–100, Intel 7–8, MovieLens λ = 0.75 n = 500: 60–130 and
   220–250), and on GP n = 100 and hypergraph n = 500 the LP is never tighter.

## Phase 3 (not run — waiting for approval)

Using every greedy prefix at every k costs almost nothing extra for NM-Dual itself: the DP rows of a
prefix already cover all k ≤ k_max, and on 18 of 24 instances greedy stops before k_max, so there
are no new prefixes at all. New prefixes (and their `high_cap_U` calls) appear only on wiki-Vote
(456, ≈ 14 min extra), Facebook (69, ≈ 4 min), YouTube (78, ≈ 35 s), MAG-10 (114) and MovieLens
λ = 0.75 n = 500 (62) (seconds each); the `--baselines` run computes the caps twice, so double these.
The cost is in the LPs: every k would use every base set, so B2 and B5 at small k become as large as
the largest LP now (or larger). Estimated summed B5 solve time per seed: Facebook ≈ 9,100 s,
wiki-Vote ≈ 2,700 s (2.27M columns per LP), YouTube ≈ 1,400 s, hypergraph n = 500 ≈ 1,050 s,
MAG-10 ≈ 930 s, MovieLens n = 500 ≈ 340–540 s, email-Eu-core ≈ 430 s; everything else < 20 s.
Full table: `phase3_estimate.md` (`common/phase3_estimate.py`).

# Tables (generated by `common/hybrid_report.py`)

## 1. Checks

| problem | rows (LPs) | non-optimal solves | check 1 rows (OPT) | check 1 violations | check 2 rows | check 2 violations | check 3 rows (certificate) | check 3 failures | _mono check 1 (4 bounds) | _mono violations |
|---|---|---|---|---|---|---|---|---|---|---|
| max_cut | 200 | 0 | 200 | 0 | 200 | 0 | 200 | 0 | 800 | 0 |
| directed_cut | 352 | 0 | 207 | 0 | 352 | 0 | 207 | 0 | 828 | 0 |
| revenue_max | 431 | 0 | 211 | 0 | 431 | 0 | 211 | 0 | 844 | 0 |
| diverse_rec | 556 | 0 | 406 | 0 | 556 | 0 | 406 | 0 | 1624 | 0 |
| log_det | 784 | 0 | 234 | 0 | 784 | 0 | 234 | 0 | 936 | 0 |
| gaussian_mi | 502 | 0 | 220 | 0 | 502 | 0 | 220 | 0 | 880 | 0 |
| hypergraph_cut | 361 | 0 | 206 | 0 | 361 | 0 | 206 | 0 | 824 | 0 |

Check 3 builds the proof's feasible point (x = 1_O, z = f(O), P^S_i = f_S(O' in the first i positions)) for the brute-force optimal set O and verifies C0–C3; it ran on every row where brute force produced an optimal set.

## 2. 2×2 ablation: mean bound / ref over k > 1

Columns: Marginal (no LP coupling, no caps), NM-Dual (caps, no coupling), B2 (coupling, no caps), Hybrid (coupling + caps), and min(NM-Dual, Hybrid).

| problem | instance | n | ref | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) | tightest |
|---|---|---|---|---|---|---|---|---|---|
| max_cut | k_sweep_n20_p0.3 | 20 | OPT | 1.633 | 1.483 | 1.303 | 1.291 | 1.284 | min(NM-Dual, Hybrid) |
| directed_cut | email_eu_core | 1005 | best | 1.834 | 1.595 | 1.521 | 1.492 | 1.492 | min(NM-Dual, Hybrid) |
| directed_cut | synthetic_n200 | 200 | best | 1.265 | 1.191 | 1.208 | 1.186 | 1.184 | min(NM-Dual, Hybrid) |
| directed_cut | synthetic_n20 | 20 | OPT | 1.316 | 1.175 | 1.146 | 1.122 | 1.116 | min(NM-Dual, Hybrid) |
| directed_cut | wiki_vote | 7115 | best | 1.059 | 1.038 | 1.038 | 1.036 | 1.035 | min(NM-Dual, Hybrid) |
| revenue_max | facebook | 4039 | best | 1.293 | 1.262 | 1.275 | 1.258 | 1.257 | min(NM-Dual, Hybrid) |
| revenue_max | synthetic_n200 | 200 | best | 1.588 | 1.549 | 1.541 | 1.531 | 1.530 | min(NM-Dual, Hybrid) |
| revenue_max | synthetic_n20 | 20 | OPT | 1.512 | 1.458 | 1.395 | 1.390 | 1.388 | min(NM-Dual, Hybrid) |
| revenue_max | youtube | 3804 | best | 1.251 | 1.160 | 1.157 | 1.139 | 1.139 | min(NM-Dual, Hybrid) |
| diverse_rec | ml20_lambda0.75 | 20 | OPT | 1.563 | 1.532 | 1.416 | 1.412 | 1.409 | min(NM-Dual, Hybrid) |
| diverse_rec | ml20_lambda1.0 | 20 | OPT | 2.080 | 2.024 | 1.861 | 1.853 | 1.850 | min(NM-Dual, Hybrid) |
| diverse_rec | ml500_lambda0.75 | 500 | best | 1.360 | 1.303 | 1.302 | 1.290 | 1.290 | min(NM-Dual, Hybrid) |
| diverse_rec | ml500_lambda1.0 | 500 | best | 1.569 | 1.493 | 1.506 | 1.484 | 1.484 | min(NM-Dual, Hybrid) |
| log_det | synthetic_n20_alpha3 | 20 | OPT | 2.456 | 2.437 | 1.902 | 1.902 | 1.902 | B2 |
| log_det | wine_n100_alpha10 | 100 | best | 1.403 | 1.382 | 1.328 | 1.327 | 1.327 | Hybrid |
| log_det | wine_n100_alpha3 | 100 | best | 2.146 | 2.115 | 1.908 | 1.908 | 1.908 | Hybrid |
| log_det | wine_n300_alpha10 | 300 | best | 1.635 | 1.618 | 1.533 | 1.532 | 1.532 | Hybrid |
| log_det | wine_n300_alpha3 | 300 | best | 2.582 | 2.559 | 2.316 | 2.316 | 2.316 | Hybrid |
| gaussian_mi | intel | 52 | best | 2.287 | 2.098 | 2.148 | 2.092 | 2.090 | min(NM-Dual, Hybrid) |
| gaussian_mi | synthetic_gp_n100 | 100 | best | 1.415 | 1.391 | 1.403 | 1.389 | 1.389 | min(NM-Dual, Hybrid) |
| gaussian_mi | synthetic_gp_n20 | 20 | OPT | 1.710 | 1.605 | 1.339 | 1.336 | 1.333 | min(NM-Dual, Hybrid) |
| hypergraph_cut | mag10_top1000 | 1000 | best | 1.326 | 1.188 | 1.212 | 1.182 | 1.181 | min(NM-Dual, Hybrid) |
| hypergraph_cut | synthetic_n20 | 20 | OPT | 1.936 | 1.777 | 1.747 | 1.722 | 1.715 | min(NM-Dual, Hybrid) |
| hypergraph_cut | synthetic_n500 | 500 | best | 1.592 | 1.462 | 1.506 | 1.455 | 1.454 | min(NM-Dual, Hybrid) |

## 3. How much do the caps tighten the LP? (B2 − Hybrid) / B2, rows with k > 1

| problem | rows | mean | max | rows Hybrid < B2 by > 1% |
|---|---|---|---|---|
| max_cut | 190 | 0.96% | 5.46% | 31% |
| directed_cut | 335 | 1.84% | 8.08% | 56% |
| revenue_max | 410 | 0.77% | 3.95% | 34% |
| diverse_rec | 530 | 0.58% | 2.69% | 25% |
| log_det | 760 | 0.03% | 0.92% | 0% |
| gaussian_mi | 486 | 0.82% | 7.38% | 36% |
| hypergraph_cut | 345 | 2.23% | 7.06% | 63% |

Per instance:

| problem | instance | mean | max | rows > 1% |
|---|---|---|---|---|
| max_cut | k_sweep_n20_p0.3 | 0.96% | 5.46% | 31% |
| directed_cut | email_eu_core | 2.11% | 6.53% | 48% |
| directed_cut | synthetic_n200 | 1.70% | 3.97% | 58% |
| directed_cut | synthetic_n20 | 2.04% | 8.08% | 61% |
| directed_cut | wiki_vote | 0.26% | 0.67% | 0% |
| revenue_max | facebook | 1.34% | 2.10% | 77% |
| revenue_max | synthetic_n200 | 0.66% | 2.20% | 37% |
| revenue_max | synthetic_n20 | 0.38% | 3.95% | 11% |
| revenue_max | youtube | 1.58% | 3.10% | 57% |
| diverse_rec | ml20_lambda0.75 | 0.30% | 2.13% | 13% |
| diverse_rec | ml20_lambda1.0 | 0.46% | 2.19% | 21% |
| diverse_rec | ml500_lambda0.75 | 0.83% | 2.04% | 36% |
| diverse_rec | ml500_lambda1.0 | 1.34% | 2.69% | 56% |
| log_det | synthetic_n20_alpha3 | 0.00% | 0.00% | 0% |
| log_det | wine_n100_alpha10 | 0.07% | 0.92% | 0% |
| log_det | wine_n100_alpha3 | 0.01% | 0.29% | 0% |
| log_det | wine_n300_alpha10 | 0.02% | 0.22% | 0% |
| log_det | wine_n300_alpha3 | 0.00% | 0.01% | 0% |
| gaussian_mi | intel | 2.48% | 4.27% | 90% |
| gaussian_mi | synthetic_gp_n100 | 0.89% | 2.11% | 44% |
| gaussian_mi | synthetic_gp_n20 | 0.28% | 7.38% | 11% |
| hypergraph_cut | mag10_top1000 | 2.46% | 3.81% | 87% |
| hypergraph_cut | synthetic_n20 | 1.56% | 7.06% | 48% |
| hypergraph_cut | synthetic_n500 | 3.18% | 5.70% | 79% |

## 4. Where min(NM-Dual, Hybrid) beats both NM-Dual alone and B2 alone (k > 1)

Gain = (min(NM-Dual, B2) − min(NM-Dual, Hybrid)) / min(NM-Dual, B2); a row counts when the gain exceeds 1e-6. Since Hybrid ≤ B2, this happens exactly when Hybrid < NM-Dual and Hybrid < B2.

| problem | instance | rows | rows beating both | mean gain (winning rows) | max gain | k range of wins |
|---|---|---|---|---|---|---|
| max_cut | k_sweep_n20_p0.3 | 190 | 15% | 0.56% | 3.13% | 2–12 |
| directed_cut | email_eu_core | 25 | 60% | 1.02% | 2.33% | 40–320 |
| directed_cut | synthetic_n200 | 100 | 55% | 0.78% | 2.02% | 30–100 |
| directed_cut | synthetic_n20 | 190 | 68% | 1.85% | 5.19% | 2–20 |
| directed_cut | wiki_vote | 20 | 70% | 0.33% | 0.67% | 70–200 |
| revenue_max | facebook | 60 | 90% | 0.25% | 0.96% | 20–200 |
| revenue_max | synthetic_n200 | 100 | 57% | 0.38% | 1.20% | 10–70 |
| revenue_max | synthetic_n20 | 190 | 53% | 0.35% | 2.44% | 2–20 |
| revenue_max | youtube | 60 | 98% | 1.01% | 1.70% | 10–200 |
| diverse_rec | ml20_lambda0.75 | 190 | 22% | 0.40% | 0.98% | 2–12 |
| diverse_rec | ml20_lambda1.0 | 190 | 23% | 0.29% | 0.97% | 2–12 |
| diverse_rec | ml500_lambda0.75 | 75 | 84% | 0.79% | 1.72% | 50–250 |
| diverse_rec | ml500_lambda1.0 | 75 | 79% | 0.55% | 1.15% | 60–250 |
| log_det | synthetic_n20_alpha3 | 190 | 0% | – | 0.00% | – |
| log_det | wine_n100_alpha10 | 245 | 29% | 0.24% | 0.92% | 16–35 |
| log_det | wine_n100_alpha3 | 245 | 20% | 0.03% | 0.29% | 16–50 |
| log_det | wine_n300_alpha10 | 40 | 25% | 0.07% | 0.22% | 35–60 |
| log_det | wine_n300_alpha3 | 40 | 35% | 0.00% | 0.01% | 35–100 |
| gaussian_mi | intel | 51 | 63% | 0.47% | 0.84% | 5–52 |
| gaussian_mi | synthetic_gp_n100 | 245 | 22% | 0.46% | 2.11% | 3–50 |
| gaussian_mi | synthetic_gp_n20 | 190 | 21% | 0.21% | 0.84% | 3–11 |
| hypergraph_cut | mag10_top1000 | 30 | 70% | 0.71% | 1.59% | 100–300 |
| hypergraph_cut | synthetic_n20 | 190 | 42% | 0.94% | 2.45% | 2–20 |
| hypergraph_cut | synthetic_n500 | 125 | 71% | 0.65% | 2.10% | 60–250 |

## 5. Phase 2: bound_k ← min over k' ≥ k (same instance and seed), rows with k > 1

| problem | rows | NM-Dual: rows changed | NM-Dual: mean / max reduction | B2: rows changed | B2: mean / max reduction | Hybrid: rows changed | Hybrid: mean / max reduction | min: rows changed | min: mean / max reduction |
|---|---|---|---|---|---|---|---|---|---|
| max_cut | 190 | 0% | 0.00% / 0.00% | 4% | 0.12% / 5.92% | 3% | 0.11% / 5.14% | 3% | 0.11% / 5.14% |
| directed_cut | 335 | 0% | 0.00% / 0.00% | 0% | 0.00% / 0.86% | 0% | 0.00% / 0.00% | 0% | 0.00% / 0.00% |
| revenue_max | 410 | 0% | 0.00% / 0.00% | 0% | 0.00% / 0.78% | 0% | 0.00% / 0.83% | 0% | 0.00% / 0.83% |
| diverse_rec | 530 | 0% | 0.00% / 0.00% | 4% | 0.07% / 3.07% | 3% | 0.05% / 3.07% | 3% | 0.05% / 3.07% |
| log_det | 760 | 0% | 0.00% / 0.00% | 0% | 0.00% / 0.18% | 0% | 0.00% / 0.18% | 0% | 0.00% / 0.18% |
| gaussian_mi | 486 | 0% | 0.00% / 0.00% | 0% | 0.00% / 1.15% | 0% | 0.00% / 1.13% | 0% | 0.00% / 1.13% |
| hypergraph_cut | 345 | 0% | 0.00% / 0.00% | 0% | 0.00% / 0.00% | 0% | 0.00% / 0.00% | 0% | 0.00% / 0.00% |

## 6. Hybrid LP sizes and runtimes

Per instance (mean over seeds): largest LP (rows × columns), hybrid export (C++: caps recomputation and file), HiGHS time summed over all k and per LP, NM-Dual time for the largest k, and B2 solve time summed over all k.

| problem | instance | n | max rows | max cols | export [s] | hybrid solve, all k [s] | per LP [s] | NM-Dual [s] | B2 solve, all k [s] |
|---|---|---|---|---|---|---|---|---|---|
| max_cut | k_sweep_n20_p0.3 | 20 | 187 | 195 | 0.03 | 0.03 | 0.00 | 0.00 | 0.01 |
| directed_cut | email_eu_core | 1005 | 259066 | 259768 | 14.35 | 192.90 | 7.42 | 15.36 | 1.00 |
| directed_cut | synthetic_n200 | 200 | 15252 | 15351 | 0.18 | 5.00 | 0.24 | 0.09 | 0.04 |
| directed_cut | synthetic_n20 | 20 | 187 | 195 | 0.02 | 0.03 | 0.00 | 0.00 | 0.01 |
| directed_cut | wiki_vote | 7115 | 147337 | 154431 | 50.52 | 95.57 | 4.55 | 49.51 | 0.43 |
| revenue_max | facebook | 4039 | 82741 | 86759 | 67.22 | 736.06 | 35.05 | 81.12 | 0.43 |
| revenue_max | synthetic_n200 | 200 | 11122 | 11256 | 0.23 | 6.65 | 0.32 | 0.13 | 0.03 |
| revenue_max | synthetic_n20 | 20 | 154 | 165 | 0.04 | 0.03 | 0.00 | 0.00 | 0.00 |
| revenue_max | youtube | 3804 | 151906 | 155669 | 25.11 | 170.86 | 8.14 | 23.66 | 0.42 |
| diverse_rec | ml20_lambda0.75 | 20 | 204 | 210 | 0.03 | 0.03 | 0.00 | 0.00 | 0.01 |
| diverse_rec | ml20_lambda1.0 | 20 | 177 | 186 | 0.04 | 0.03 | 0.00 | 0.00 | 0.01 |
| diverse_rec | ml500_lambda0.75 | 500 | 94377 | 94626 | 9.36 | 214.87 | 8.26 | 9.07 | 0.26 |
| diverse_rec | ml500_lambda1.0 | 500 | 88624 | 88895 | 8.91 | 146.21 | 5.62 | 8.64 | 0.25 |
| log_det | synthetic_n20_alpha3 | 20 | 112 | 126 | 0.33 | 0.02 | 0.00 | 0.00 | 0.01 |
| log_det | wine_n100_alpha10 | 100 | 2941 | 3006 | 3.24 | 1.34 | 0.03 | 3.06 | 0.03 |
| log_det | wine_n100_alpha3 | 100 | 1666 | 1748 | 1.39 | 0.55 | 0.01 | 1.32 | 0.03 |
| log_det | wine_n300_alpha10 | 300 | 3286 | 3574 | 77.96 | 0.94 | 0.04 | 86.98 | 0.02 |
| log_det | wine_n300_alpha3 | 300 | 1734 | 2028 | 50.89 | 0.38 | 0.02 | 45.11 | 0.02 |
| gaussian_mi | intel | 52 | 1081 | 1106 | 10.14 | 0.39 | 0.01 | 0.18 | 0.03 |
| gaussian_mi | synthetic_gp_n100 | 100 | 3877 | 3926 | 7.42 | 1.77 | 0.04 | 4.82 | 0.04 |
| gaussian_mi | synthetic_gp_n20 | 20 | 187 | 195 | 0.59 | 0.03 | 0.00 | 0.00 | 0.01 |
| hypergraph_cut | mag10_top1000 | 1000 | 256152 | 256851 | 8.64 | 262.56 | 8.47 | 8.56 | 0.97 |
| hypergraph_cut | synthetic_n20 | 20 | 187 | 195 | 0.04 | 0.03 | 0.00 | 0.00 | 0.01 |
| hypergraph_cut | synthetic_n500 | 500 | 91299 | 91560 | 1.46 | 267.52 | 10.29 | 1.48 | 0.26 |

## Appendix: per-k ablation (mean bound / ref over seeds; k = 1 omitted)

### max_cut / k_sweep_n20_p0.3 (n=20, ref = OPT)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.066 | 1.024 | 1.055 | 1.050 | 1.023 |
| 3 | 1.121 | 1.061 | 1.098 | 1.086 | 1.060 |
| 4 | 1.171 | 1.091 | 1.134 | 1.113 | 1.090 |
| 5 | 1.223 | 1.124 | 1.175 | 1.144 | 1.124 |
| 6 | 1.288 | 1.165 | 1.226 | 1.179 | 1.164 |
| 7 | 1.363 | 1.216 | 1.276 | 1.225 | 1.214 |
| 8 | 1.447 | 1.278 | 1.319 | 1.280 | 1.273 |
| 9 | 1.535 | 1.349 | 1.347 | 1.330 | 1.330 |
| 10 | 1.653 | 1.446 | 1.374 | 1.370 | 1.370 |
| 11 | 1.772 | 1.543 | 1.371 | 1.371 | 1.371 |
| 12 | 1.875 | 1.633 | 1.375 | 1.375 | 1.375 |
| 13 | 1.929 | 1.715 | 1.375 | 1.375 | 1.375 |
| 14 | 1.940 | 1.772 | 1.375 | 1.375 | 1.375 |
| 15 | 1.941 | 1.792 | 1.375 | 1.375 | 1.375 |
| 16 | 1.941 | 1.794 | 1.375 | 1.375 | 1.375 |
| 17 | 1.941 | 1.794 | 1.375 | 1.375 | 1.375 |
| 18 | 1.941 | 1.794 | 1.375 | 1.375 | 1.375 |
| 19 | 1.941 | 1.794 | 1.375 | 1.375 | 1.375 |
| 20 | 1.941 | 1.794 | 1.375 | 1.375 | 1.375 |

### directed_cut / email_eu_core (n=1005, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 20 | 1.074 | 1.054 | 1.060 | 1.058 | 1.054 |
| 40 | 1.181 | 1.130 | 1.145 | 1.126 | 1.126 |
| 60 | 1.262 | 1.163 | 1.197 | 1.158 | 1.158 |
| 80 | 1.350 | 1.193 | 1.254 | 1.187 | 1.187 |
| 100 | 1.436 | 1.230 | 1.303 | 1.222 | 1.222 |
| 120 | 1.521 | 1.275 | 1.352 | 1.264 | 1.264 |
| 140 | 1.597 | 1.320 | 1.392 | 1.304 | 1.304 |
| 160 | 1.673 | 1.367 | 1.433 | 1.348 | 1.348 |
| 180 | 1.748 | 1.417 | 1.471 | 1.393 | 1.393 |
| 200 | 1.817 | 1.468 | 1.505 | 1.439 | 1.439 |
| 220 | 1.875 | 1.518 | 1.537 | 1.485 | 1.485 |
| 240 | 1.927 | 1.566 | 1.566 | 1.530 | 1.530 |
| 260 | 1.977 | 1.614 | 1.594 | 1.572 | 1.572 |
| 280 | 2.022 | 1.663 | 1.618 | 1.610 | 1.610 |
| 300 | 2.056 | 1.710 | 1.636 | 1.634 | 1.634 |
| 320 | 2.084 | 1.756 | 1.654 | 1.654 | 1.654 |
| 340 | 2.109 | 1.799 | 1.669 | 1.669 | 1.669 |
| 360 | 2.129 | 1.839 | 1.682 | 1.682 | 1.682 |
| 380 | 2.136 | 1.877 | 1.693 | 1.693 | 1.693 |
| 400 | 2.142 | 1.913 | 1.702 | 1.702 | 1.702 |
| 420 | 2.146 | 1.946 | 1.708 | 1.708 | 1.708 |
| 440 | 2.148 | 1.976 | 1.712 | 1.712 | 1.712 |
| 460 | 2.150 | 2.002 | 1.716 | 1.716 | 1.716 |
| 480 | 2.150 | 2.024 | 1.717 | 1.717 | 1.717 |
| 500 | 2.150 | 2.044 | 1.719 | 1.719 | 1.719 |

### directed_cut / synthetic_n200 (n=200, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 5 | 1.011 | 1.003 | 1.010 | 1.010 | 1.003 |
| 10 | 1.022 | 1.011 | 1.019 | 1.018 | 1.011 |
| 15 | 1.043 | 1.031 | 1.038 | 1.037 | 1.031 |
| 20 | 1.065 | 1.050 | 1.057 | 1.055 | 1.050 |
| 25 | 1.086 | 1.069 | 1.075 | 1.072 | 1.069 |
| 30 | 1.109 | 1.087 | 1.094 | 1.089 | 1.087 |
| 35 | 1.134 | 1.108 | 1.115 | 1.109 | 1.107 |
| 40 | 1.160 | 1.128 | 1.137 | 1.128 | 1.127 |
| 45 | 1.188 | 1.145 | 1.159 | 1.145 | 1.144 |
| 50 | 1.218 | 1.162 | 1.183 | 1.161 | 1.161 |
| 55 | 1.249 | 1.180 | 1.207 | 1.178 | 1.177 |
| 60 | 1.281 | 1.198 | 1.231 | 1.195 | 1.195 |
| 65 | 1.316 | 1.221 | 1.257 | 1.216 | 1.215 |
| 70 | 1.352 | 1.245 | 1.284 | 1.238 | 1.238 |
| 75 | 1.391 | 1.274 | 1.312 | 1.264 | 1.264 |
| 80 | 1.433 | 1.304 | 1.339 | 1.292 | 1.292 |
| 85 | 1.479 | 1.338 | 1.368 | 1.324 | 1.324 |
| 90 | 1.529 | 1.376 | 1.398 | 1.359 | 1.359 |
| 95 | 1.584 | 1.420 | 1.427 | 1.396 | 1.396 |
| 100 | 1.644 | 1.467 | 1.452 | 1.433 | 1.433 |

### directed_cut / synthetic_n20 (n=20, ref = OPT)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.034 | 1.007 | 1.024 | 1.022 | 1.004 |
| 3 | 1.047 | 1.012 | 1.031 | 1.029 | 1.012 |
| 4 | 1.074 | 1.022 | 1.046 | 1.042 | 1.021 |
| 5 | 1.107 | 1.037 | 1.063 | 1.054 | 1.036 |
| 6 | 1.154 | 1.057 | 1.089 | 1.073 | 1.055 |
| 7 | 1.204 | 1.075 | 1.116 | 1.087 | 1.073 |
| 8 | 1.272 | 1.106 | 1.144 | 1.106 | 1.097 |
| 9 | 1.330 | 1.137 | 1.163 | 1.121 | 1.117 |
| 10 | 1.387 | 1.183 | 1.182 | 1.145 | 1.144 |
| 11 | 1.422 | 1.223 | 1.188 | 1.159 | 1.159 |
| 12 | 1.432 | 1.250 | 1.192 | 1.163 | 1.163 |
| 13 | 1.440 | 1.267 | 1.192 | 1.165 | 1.165 |
| 14 | 1.442 | 1.277 | 1.192 | 1.165 | 1.165 |
| 15 | 1.442 | 1.278 | 1.192 | 1.165 | 1.165 |
| 16 | 1.442 | 1.278 | 1.192 | 1.165 | 1.165 |
| 17 | 1.442 | 1.278 | 1.192 | 1.165 | 1.165 |
| 18 | 1.442 | 1.278 | 1.192 | 1.165 | 1.165 |
| 19 | 1.442 | 1.278 | 1.192 | 1.165 | 1.165 |
| 20 | 1.442 | 1.278 | 1.192 | 1.165 | 1.165 |

### directed_cut / wiki_vote (n=7115, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 10 | 1.005 | 1.000 | 1.005 | 1.004 | 1.000 |
| 20 | 1.010 | 1.002 | 1.008 | 1.008 | 1.002 |
| 30 | 1.014 | 1.005 | 1.010 | 1.010 | 1.005 |
| 40 | 1.022 | 1.013 | 1.016 | 1.016 | 1.013 |
| 50 | 1.028 | 1.018 | 1.020 | 1.019 | 1.018 |
| 60 | 1.035 | 1.023 | 1.024 | 1.024 | 1.023 |
| 70 | 1.040 | 1.027 | 1.028 | 1.027 | 1.027 |
| 80 | 1.044 | 1.030 | 1.030 | 1.029 | 1.029 |
| 90 | 1.050 | 1.033 | 1.034 | 1.032 | 1.032 |
| 100 | 1.055 | 1.037 | 1.036 | 1.034 | 1.034 |
| 110 | 1.061 | 1.041 | 1.040 | 1.037 | 1.037 |
| 120 | 1.067 | 1.045 | 1.044 | 1.041 | 1.041 |
| 130 | 1.074 | 1.051 | 1.048 | 1.046 | 1.046 |
| 140 | 1.081 | 1.055 | 1.052 | 1.049 | 1.049 |
| 150 | 1.088 | 1.058 | 1.056 | 1.052 | 1.052 |
| 160 | 1.093 | 1.061 | 1.058 | 1.054 | 1.054 |
| 170 | 1.098 | 1.063 | 1.061 | 1.056 | 1.056 |
| 180 | 1.103 | 1.065 | 1.064 | 1.057 | 1.057 |
| 190 | 1.108 | 1.067 | 1.066 | 1.059 | 1.059 |
| 200 | 1.112 | 1.068 | 1.067 | 1.060 | 1.060 |

### revenue_max / facebook (n=4039, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 10 | 1.136 | 1.095 | 1.118 | 1.107 | 1.095 |
| 20 | 1.218 | 1.209 | 1.173 | 1.169 | 1.169 |
| 30 | 1.236 | 1.217 | 1.215 | 1.205 | 1.205 |
| 40 | 1.244 | 1.230 | 1.234 | 1.222 | 1.222 |
| 50 | 1.248 | 1.233 | 1.243 | 1.233 | 1.233 |
| 60 | 1.258 | 1.237 | 1.249 | 1.237 | 1.237 |
| 70 | 1.270 | 1.245 | 1.259 | 1.244 | 1.244 |
| 80 | 1.283 | 1.253 | 1.269 | 1.252 | 1.252 |
| 90 | 1.296 | 1.263 | 1.279 | 1.260 | 1.260 |
| 100 | 1.305 | 1.272 | 1.289 | 1.268 | 1.268 |
| 110 | 1.310 | 1.280 | 1.296 | 1.276 | 1.276 |
| 120 | 1.315 | 1.286 | 1.302 | 1.282 | 1.282 |
| 130 | 1.321 | 1.291 | 1.307 | 1.288 | 1.288 |
| 140 | 1.327 | 1.295 | 1.311 | 1.293 | 1.293 |
| 150 | 1.335 | 1.299 | 1.316 | 1.297 | 1.297 |
| 160 | 1.341 | 1.302 | 1.320 | 1.300 | 1.300 |
| 170 | 1.346 | 1.305 | 1.325 | 1.302 | 1.302 |
| 180 | 1.350 | 1.308 | 1.329 | 1.305 | 1.305 |
| 190 | 1.355 | 1.311 | 1.334 | 1.308 | 1.308 |
| 200 | 1.359 | 1.314 | 1.338 | 1.311 | 1.311 |

### revenue_max / synthetic_n200 (n=200, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 5 | 1.149 | 1.130 | 1.138 | 1.136 | 1.130 |
| 10 | 1.371 | 1.329 | 1.333 | 1.327 | 1.325 |
| 15 | 1.560 | 1.494 | 1.502 | 1.485 | 1.485 |
| 20 | 1.581 | 1.531 | 1.541 | 1.523 | 1.523 |
| 25 | 1.589 | 1.530 | 1.549 | 1.522 | 1.522 |
| 30 | 1.583 | 1.534 | 1.552 | 1.525 | 1.525 |
| 35 | 1.586 | 1.530 | 1.552 | 1.525 | 1.525 |
| 40 | 1.596 | 1.536 | 1.557 | 1.532 | 1.532 |
| 45 | 1.610 | 1.548 | 1.565 | 1.542 | 1.542 |
| 50 | 1.619 | 1.561 | 1.573 | 1.555 | 1.555 |
| 55 | 1.631 | 1.579 | 1.581 | 1.571 | 1.571 |
| 60 | 1.645 | 1.595 | 1.586 | 1.584 | 1.584 |
| 65 | 1.653 | 1.616 | 1.592 | 1.591 | 1.591 |
| 70 | 1.655 | 1.633 | 1.598 | 1.598 | 1.598 |
| 75 | 1.655 | 1.639 | 1.600 | 1.600 | 1.600 |
| 80 | 1.656 | 1.640 | 1.600 | 1.600 | 1.600 |
| 85 | 1.656 | 1.640 | 1.600 | 1.600 | 1.600 |
| 90 | 1.656 | 1.640 | 1.600 | 1.600 | 1.600 |
| 95 | 1.656 | 1.640 | 1.600 | 1.600 | 1.600 |
| 100 | 1.656 | 1.640 | 1.600 | 1.600 | 1.600 |

### revenue_max / synthetic_n20 (n=20, ref = OPT)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.185 | 1.137 | 1.138 | 1.136 | 1.125 |
| 3 | 1.384 | 1.254 | 1.271 | 1.249 | 1.238 |
| 4 | 1.505 | 1.353 | 1.382 | 1.349 | 1.345 |
| 5 | 1.540 | 1.433 | 1.427 | 1.407 | 1.406 |
| 6 | 1.538 | 1.469 | 1.430 | 1.426 | 1.426 |
| 7 | 1.537 | 1.487 | 1.421 | 1.420 | 1.420 |
| 8 | 1.541 | 1.500 | 1.418 | 1.417 | 1.417 |
| 9 | 1.542 | 1.504 | 1.418 | 1.417 | 1.417 |
| 10 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 11 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 12 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 13 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 14 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 15 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 16 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 17 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 18 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 19 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |
| 20 | 1.542 | 1.506 | 1.418 | 1.417 | 1.417 |

### revenue_max / youtube (n=3804, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 10 | 1.225 | 1.142 | 1.147 | 1.139 | 1.139 |
| 20 | 1.284 | 1.228 | 1.188 | 1.185 | 1.185 |
| 30 | 1.283 | 1.210 | 1.188 | 1.185 | 1.185 |
| 40 | 1.266 | 1.205 | 1.179 | 1.176 | 1.176 |
| 50 | 1.257 | 1.191 | 1.169 | 1.162 | 1.162 |
| 60 | 1.254 | 1.178 | 1.158 | 1.152 | 1.152 |
| 70 | 1.247 | 1.169 | 1.151 | 1.144 | 1.144 |
| 80 | 1.245 | 1.162 | 1.146 | 1.138 | 1.138 |
| 90 | 1.247 | 1.156 | 1.145 | 1.134 | 1.134 |
| 100 | 1.247 | 1.151 | 1.146 | 1.131 | 1.131 |
| 110 | 1.246 | 1.147 | 1.147 | 1.128 | 1.128 |
| 120 | 1.246 | 1.144 | 1.149 | 1.126 | 1.126 |
| 130 | 1.246 | 1.141 | 1.150 | 1.124 | 1.124 |
| 140 | 1.245 | 1.139 | 1.151 | 1.123 | 1.123 |
| 150 | 1.245 | 1.138 | 1.152 | 1.122 | 1.122 |
| 160 | 1.246 | 1.137 | 1.153 | 1.122 | 1.122 |
| 170 | 1.247 | 1.137 | 1.155 | 1.122 | 1.122 |
| 180 | 1.248 | 1.138 | 1.156 | 1.123 | 1.123 |
| 190 | 1.250 | 1.138 | 1.158 | 1.124 | 1.124 |
| 200 | 1.250 | 1.139 | 1.160 | 1.124 | 1.124 |

### diverse_rec / ml20_lambda0.75 (n=20, ref = OPT)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.052 | 1.038 | 1.049 | 1.049 | 1.038 |
| 3 | 1.102 | 1.085 | 1.097 | 1.096 | 1.085 |
| 4 | 1.156 | 1.138 | 1.147 | 1.146 | 1.138 |
| 5 | 1.215 | 1.194 | 1.202 | 1.200 | 1.194 |
| 6 | 1.280 | 1.250 | 1.260 | 1.257 | 1.250 |
| 7 | 1.348 | 1.305 | 1.322 | 1.311 | 1.305 |
| 8 | 1.424 | 1.367 | 1.387 | 1.369 | 1.367 |
| 9 | 1.508 | 1.438 | 1.455 | 1.436 | 1.434 |
| 10 | 1.598 | 1.516 | 1.522 | 1.506 | 1.506 |
| 11 | 1.699 | 1.603 | 1.589 | 1.579 | 1.579 |
| 12 | 1.805 | 1.694 | 1.594 | 1.592 | 1.592 |
| 13 | 1.817 | 1.793 | 1.537 | 1.537 | 1.537 |
| 14 | 1.814 | 1.811 | 1.535 | 1.535 | 1.535 |
| 15 | 1.814 | 1.811 | 1.535 | 1.535 | 1.535 |
| 16 | 1.814 | 1.811 | 1.535 | 1.535 | 1.535 |
| 17 | 1.814 | 1.811 | 1.535 | 1.535 | 1.535 |
| 18 | 1.814 | 1.811 | 1.535 | 1.535 | 1.535 |
| 19 | 1.814 | 1.811 | 1.535 | 1.535 | 1.535 |
| 20 | 1.814 | 1.811 | 1.535 | 1.535 | 1.535 |

### diverse_rec / ml20_lambda1.0 (n=20, ref = OPT)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.071 | 1.056 | 1.069 | 1.068 | 1.056 |
| 3 | 1.145 | 1.126 | 1.139 | 1.138 | 1.126 |
| 4 | 1.226 | 1.202 | 1.216 | 1.213 | 1.202 |
| 5 | 1.318 | 1.281 | 1.304 | 1.294 | 1.281 |
| 6 | 1.422 | 1.374 | 1.400 | 1.384 | 1.374 |
| 7 | 1.540 | 1.483 | 1.511 | 1.489 | 1.483 |
| 8 | 1.678 | 1.608 | 1.637 | 1.611 | 1.607 |
| 9 | 1.833 | 1.752 | 1.776 | 1.750 | 1.749 |
| 10 | 2.009 | 1.915 | 1.929 | 1.908 | 1.908 |
| 11 | 2.195 | 2.085 | 2.080 | 2.068 | 2.068 |
| 12 | 2.377 | 2.252 | 2.217 | 2.212 | 2.212 |
| 13 | 2.556 | 2.416 | 2.260 | 2.260 | 2.260 |
| 14 | 2.721 | 2.571 | 2.260 | 2.260 | 2.260 |
| 15 | 2.739 | 2.715 | 2.260 | 2.260 | 2.260 |
| 16 | 2.739 | 2.724 | 2.260 | 2.260 | 2.260 |
| 17 | 2.739 | 2.724 | 2.260 | 2.260 | 2.260 |
| 18 | 2.739 | 2.724 | 2.260 | 2.260 | 2.260 |
| 19 | 2.739 | 2.724 | 2.260 | 2.260 | 2.260 |
| 20 | 2.739 | 2.724 | 2.260 | 2.260 | 2.260 |

### diverse_rec / ml500_lambda0.75 (n=500, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 10 | 1.024 | 1.020 | 1.023 | 1.023 | 1.020 |
| 20 | 1.049 | 1.045 | 1.047 | 1.047 | 1.045 |
| 30 | 1.073 | 1.069 | 1.070 | 1.070 | 1.069 |
| 40 | 1.097 | 1.092 | 1.093 | 1.092 | 1.092 |
| 50 | 1.122 | 1.115 | 1.115 | 1.114 | 1.114 |
| 60 | 1.147 | 1.137 | 1.137 | 1.136 | 1.136 |
| 70 | 1.172 | 1.160 | 1.159 | 1.158 | 1.158 |
| 80 | 1.198 | 1.184 | 1.181 | 1.180 | 1.180 |
| 90 | 1.224 | 1.207 | 1.204 | 1.202 | 1.202 |
| 100 | 1.252 | 1.231 | 1.228 | 1.224 | 1.224 |
| 110 | 1.279 | 1.254 | 1.251 | 1.246 | 1.246 |
| 120 | 1.308 | 1.276 | 1.274 | 1.267 | 1.267 |
| 130 | 1.338 | 1.299 | 1.298 | 1.289 | 1.289 |
| 140 | 1.368 | 1.318 | 1.321 | 1.308 | 1.308 |
| 150 | 1.400 | 1.338 | 1.345 | 1.326 | 1.326 |
| 160 | 1.432 | 1.360 | 1.369 | 1.346 | 1.346 |
| 170 | 1.465 | 1.383 | 1.394 | 1.367 | 1.367 |
| 180 | 1.499 | 1.408 | 1.419 | 1.390 | 1.390 |
| 190 | 1.534 | 1.435 | 1.443 | 1.414 | 1.414 |
| 200 | 1.571 | 1.462 | 1.468 | 1.440 | 1.440 |
| 210 | 1.609 | 1.492 | 1.492 | 1.466 | 1.466 |
| 220 | 1.649 | 1.523 | 1.517 | 1.494 | 1.494 |
| 230 | 1.690 | 1.556 | 1.542 | 1.523 | 1.523 |
| 240 | 1.733 | 1.590 | 1.567 | 1.551 | 1.551 |
| 250 | 1.777 | 1.625 | 1.591 | 1.580 | 1.580 |

### diverse_rec / ml500_lambda1.0 (n=500, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 10 | 1.032 | 1.028 | 1.031 | 1.031 | 1.028 |
| 20 | 1.066 | 1.062 | 1.065 | 1.064 | 1.062 |
| 30 | 1.100 | 1.095 | 1.097 | 1.097 | 1.095 |
| 40 | 1.134 | 1.127 | 1.130 | 1.128 | 1.127 |
| 50 | 1.170 | 1.159 | 1.162 | 1.160 | 1.159 |
| 60 | 1.206 | 1.192 | 1.195 | 1.192 | 1.192 |
| 70 | 1.243 | 1.225 | 1.229 | 1.225 | 1.225 |
| 80 | 1.282 | 1.260 | 1.265 | 1.258 | 1.258 |
| 90 | 1.323 | 1.294 | 1.302 | 1.292 | 1.292 |
| 100 | 1.366 | 1.326 | 1.341 | 1.323 | 1.323 |
| 110 | 1.411 | 1.358 | 1.381 | 1.354 | 1.354 |
| 120 | 1.458 | 1.394 | 1.422 | 1.390 | 1.390 |
| 130 | 1.507 | 1.433 | 1.464 | 1.428 | 1.428 |
| 140 | 1.559 | 1.475 | 1.508 | 1.469 | 1.469 |
| 150 | 1.612 | 1.520 | 1.553 | 1.512 | 1.512 |
| 160 | 1.669 | 1.568 | 1.601 | 1.559 | 1.559 |
| 170 | 1.729 | 1.620 | 1.652 | 1.609 | 1.609 |
| 180 | 1.792 | 1.675 | 1.705 | 1.663 | 1.663 |
| 190 | 1.859 | 1.733 | 1.759 | 1.719 | 1.719 |
| 200 | 1.929 | 1.795 | 1.815 | 1.779 | 1.779 |
| 210 | 2.003 | 1.860 | 1.874 | 1.842 | 1.842 |
| 220 | 2.079 | 1.927 | 1.934 | 1.906 | 1.906 |
| 230 | 2.154 | 1.993 | 1.992 | 1.970 | 1.970 |
| 240 | 2.232 | 2.062 | 2.052 | 2.034 | 2.034 |
| 250 | 2.310 | 2.132 | 2.111 | 2.098 | 2.098 |

### log_det / synthetic_n20_alpha3 (n=20, ref = OPT)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.023 | 1.023 | 1.023 | 1.023 | 1.023 |
| 3 | 1.110 | 1.110 | 1.110 | 1.110 | 1.110 |
| 4 | 1.255 | 1.255 | 1.255 | 1.255 | 1.255 |
| 5 | 1.497 | 1.497 | 1.497 | 1.497 | 1.497 |
| 6 | 1.797 | 1.797 | 1.797 | 1.797 | 1.797 |
| 7 | 2.096 | 2.096 | 2.035 | 2.035 | 2.035 |
| 8 | 2.396 | 2.396 | 2.110 | 2.110 | 2.110 |
| 9 | 2.676 | 2.670 | 2.110 | 2.110 | 2.110 |
| 10 | 2.882 | 2.864 | 2.110 | 2.110 | 2.110 |
| 11 | 2.968 | 2.942 | 2.110 | 2.110 | 2.110 |
| 12 | 2.986 | 2.951 | 2.110 | 2.110 | 2.110 |
| 13 | 2.991 | 2.956 | 2.110 | 2.110 | 2.110 |
| 14 | 2.995 | 2.960 | 2.110 | 2.110 | 2.110 |
| 15 | 2.999 | 2.964 | 2.110 | 2.110 | 2.110 |
| 16 | 3.000 | 2.965 | 2.110 | 2.110 | 2.110 |
| 17 | 3.000 | 2.965 | 2.110 | 2.110 | 2.110 |
| 18 | 3.000 | 2.965 | 2.110 | 2.110 | 2.110 |
| 19 | 3.000 | 2.965 | 2.110 | 2.110 | 2.110 |
| 20 | 3.000 | 2.965 | 2.110 | 2.110 | 2.110 |

### log_det / wine_n100_alpha10 (n=100, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.000 | 1.000 | 1.000 | 1.000 | 1.000 |
| 3 | 1.001 | 1.001 | 1.001 | 1.001 | 1.001 |
| 4 | 1.009 | 1.009 | 1.009 | 1.009 | 1.009 |
| 5 | 1.018 | 1.018 | 1.018 | 1.018 | 1.018 |
| 6 | 1.033 | 1.033 | 1.033 | 1.033 | 1.033 |
| 7 | 1.049 | 1.049 | 1.049 | 1.049 | 1.049 |
| 8 | 1.069 | 1.069 | 1.069 | 1.069 | 1.069 |
| 9 | 1.092 | 1.092 | 1.092 | 1.092 | 1.092 |
| 10 | 1.119 | 1.119 | 1.119 | 1.119 | 1.119 |
| 11 | 1.143 | 1.143 | 1.143 | 1.143 | 1.143 |
| 12 | 1.171 | 1.171 | 1.171 | 1.171 | 1.171 |
| 13 | 1.198 | 1.198 | 1.198 | 1.198 | 1.198 |
| 14 | 1.225 | 1.225 | 1.225 | 1.225 | 1.225 |
| 15 | 1.250 | 1.250 | 1.250 | 1.250 | 1.250 |
| 16 | 1.278 | 1.278 | 1.275 | 1.275 | 1.275 |
| 17 | 1.306 | 1.306 | 1.294 | 1.293 | 1.293 |
| 18 | 1.337 | 1.337 | 1.312 | 1.310 | 1.310 |
| 19 | 1.372 | 1.367 | 1.329 | 1.327 | 1.327 |
| 20 | 1.406 | 1.393 | 1.345 | 1.341 | 1.341 |
| 21 | 1.441 | 1.417 | 1.359 | 1.354 | 1.354 |
| 22 | 1.475 | 1.437 | 1.372 | 1.367 | 1.367 |
| 23 | 1.506 | 1.459 | 1.384 | 1.380 | 1.380 |
| 24 | 1.537 | 1.480 | 1.396 | 1.392 | 1.392 |
| 25 | 1.568 | 1.502 | 1.406 | 1.403 | 1.403 |
| 26 | 1.576 | 1.517 | 1.415 | 1.411 | 1.411 |
| 27 | 1.582 | 1.526 | 1.423 | 1.420 | 1.420 |
| 28 | 1.584 | 1.536 | 1.432 | 1.429 | 1.429 |
| 29 | 1.577 | 1.541 | 1.439 | 1.437 | 1.437 |
| 30 | 1.570 | 1.538 | 1.445 | 1.444 | 1.444 |
| 31 | 1.565 | 1.535 | 1.448 | 1.447 | 1.447 |
| 32 | 1.564 | 1.535 | 1.452 | 1.451 | 1.451 |
| 33 | 1.563 | 1.534 | 1.453 | 1.453 | 1.453 |
| 34 | 1.562 | 1.534 | 1.454 | 1.454 | 1.454 |
| 35 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 36 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 37 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 38 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 39 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 40 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 41 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 42 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 43 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 44 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 45 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 46 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 47 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 48 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 49 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |
| 50 | 1.562 | 1.534 | 1.455 | 1.455 | 1.455 |

### log_det / wine_n100_alpha3 (n=100, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.000 | 1.000 | 1.000 | 1.000 | 1.000 |
| 3 | 1.002 | 1.002 | 1.002 | 1.002 | 1.002 |
| 4 | 1.019 | 1.019 | 1.019 | 1.019 | 1.019 |
| 5 | 1.039 | 1.039 | 1.039 | 1.039 | 1.039 |
| 6 | 1.071 | 1.071 | 1.071 | 1.071 | 1.071 |
| 7 | 1.109 | 1.109 | 1.109 | 1.109 | 1.109 |
| 8 | 1.156 | 1.156 | 1.156 | 1.156 | 1.156 |
| 9 | 1.214 | 1.214 | 1.214 | 1.214 | 1.214 |
| 10 | 1.286 | 1.286 | 1.286 | 1.286 | 1.286 |
| 11 | 1.357 | 1.357 | 1.357 | 1.357 | 1.357 |
| 12 | 1.442 | 1.442 | 1.442 | 1.442 | 1.442 |
| 13 | 1.530 | 1.530 | 1.530 | 1.530 | 1.530 |
| 14 | 1.626 | 1.626 | 1.626 | 1.626 | 1.626 |
| 15 | 1.718 | 1.718 | 1.718 | 1.718 | 1.718 |
| 16 | 1.831 | 1.831 | 1.822 | 1.822 | 1.822 |
| 17 | 1.944 | 1.943 | 1.907 | 1.907 | 1.907 |
| 18 | 2.056 | 2.047 | 1.975 | 1.974 | 1.974 |
| 19 | 2.170 | 2.129 | 2.030 | 2.029 | 2.029 |
| 20 | 2.242 | 2.187 | 2.065 | 2.064 | 2.064 |
| 21 | 2.332 | 2.265 | 2.117 | 2.116 | 2.116 |
| 22 | 2.392 | 2.317 | 2.141 | 2.140 | 2.140 |
| 23 | 2.430 | 2.365 | 2.163 | 2.163 | 2.163 |
| 24 | 2.460 | 2.408 | 2.177 | 2.177 | 2.177 |
| 25 | 2.497 | 2.453 | 2.195 | 2.194 | 2.194 |
| 26 | 2.528 | 2.484 | 2.204 | 2.203 | 2.203 |
| 27 | 2.554 | 2.509 | 2.209 | 2.209 | 2.209 |
| 28 | 2.568 | 2.525 | 2.212 | 2.212 | 2.212 |
| 29 | 2.581 | 2.537 | 2.214 | 2.214 | 2.214 |
| 30 | 2.592 | 2.549 | 2.215 | 2.215 | 2.215 |
| 31 | 2.602 | 2.559 | 2.215 | 2.215 | 2.215 |
| 32 | 2.611 | 2.569 | 2.215 | 2.215 | 2.215 |
| 33 | 2.620 | 2.577 | 2.215 | 2.215 | 2.215 |
| 34 | 2.621 | 2.578 | 2.215 | 2.215 | 2.215 |
| 35 | 2.622 | 2.578 | 2.215 | 2.215 | 2.215 |
| 36 | 2.623 | 2.578 | 2.215 | 2.215 | 2.215 |
| 37 | 2.623 | 2.578 | 2.215 | 2.215 | 2.215 |
| 38 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 39 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 40 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 41 | 2.621 | 2.576 | 2.212 | 2.212 | 2.212 |
| 42 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 43 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 44 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 45 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 46 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 47 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 48 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 49 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |
| 50 | 2.624 | 2.578 | 2.215 | 2.215 | 2.215 |

### log_det / wine_n300_alpha10 (n=300, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 5 | 1.005 | 1.005 | 1.005 | 1.005 | 1.005 |
| 10 | 1.046 | 1.046 | 1.046 | 1.046 | 1.046 |
| 15 | 1.112 | 1.112 | 1.112 | 1.112 | 1.112 |
| 20 | 1.194 | 1.194 | 1.194 | 1.194 | 1.194 |
| 25 | 1.284 | 1.284 | 1.284 | 1.284 | 1.284 |
| 30 | 1.382 | 1.382 | 1.382 | 1.382 | 1.382 |
| 35 | 1.490 | 1.490 | 1.466 | 1.463 | 1.463 |
| 40 | 1.620 | 1.603 | 1.524 | 1.523 | 1.523 |
| 45 | 1.746 | 1.695 | 1.586 | 1.584 | 1.584 |
| 50 | 1.823 | 1.768 | 1.647 | 1.647 | 1.647 |
| 55 | 1.869 | 1.818 | 1.698 | 1.698 | 1.698 |
| 60 | 1.896 | 1.861 | 1.728 | 1.728 | 1.728 |
| 65 | 1.903 | 1.878 | 1.742 | 1.742 | 1.742 |
| 70 | 1.905 | 1.884 | 1.748 | 1.748 | 1.748 |
| 75 | 1.905 | 1.887 | 1.748 | 1.748 | 1.748 |
| 80 | 1.905 | 1.889 | 1.748 | 1.748 | 1.748 |
| 85 | 1.905 | 1.890 | 1.748 | 1.748 | 1.748 |
| 90 | 1.905 | 1.890 | 1.748 | 1.748 | 1.748 |
| 95 | 1.905 | 1.890 | 1.748 | 1.748 | 1.748 |
| 100 | 1.905 | 1.890 | 1.748 | 1.748 | 1.748 |

### log_det / wine_n300_alpha3 (n=300, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 5 | 1.011 | 1.011 | 1.011 | 1.011 | 1.011 |
| 10 | 1.103 | 1.103 | 1.103 | 1.103 | 1.103 |
| 15 | 1.269 | 1.269 | 1.269 | 1.269 | 1.269 |
| 20 | 1.518 | 1.518 | 1.518 | 1.518 | 1.518 |
| 25 | 1.857 | 1.857 | 1.857 | 1.857 | 1.857 |
| 30 | 2.228 | 2.228 | 2.228 | 2.228 | 2.228 |
| 35 | 2.599 | 2.595 | 2.508 | 2.508 | 2.508 |
| 40 | 2.930 | 2.863 | 2.622 | 2.622 | 2.622 |
| 45 | 3.067 | 2.999 | 2.678 | 2.678 | 2.678 |
| 50 | 3.081 | 3.029 | 2.684 | 2.684 | 2.684 |
| 55 | 3.089 | 3.053 | 2.684 | 2.684 | 2.684 |
| 60 | 3.094 | 3.070 | 2.684 | 2.684 | 2.684 |
| 65 | 3.097 | 3.073 | 2.684 | 2.684 | 2.684 |
| 70 | 3.098 | 3.074 | 2.684 | 2.684 | 2.684 |
| 75 | 3.098 | 3.074 | 2.684 | 2.684 | 2.684 |
| 80 | 3.098 | 3.074 | 2.684 | 2.684 | 2.684 |
| 85 | 3.098 | 3.074 | 2.684 | 2.684 | 2.684 |
| 90 | 3.098 | 3.074 | 2.684 | 2.684 | 2.684 |
| 95 | 3.098 | 3.074 | 2.684 | 2.684 | 2.684 |
| 100 | 3.098 | 3.074 | 2.684 | 2.684 | 2.684 |

### gaussian_mi / intel (n=52, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.149 | 1.123 | 1.143 | 1.136 | 1.123 |
| 3 | 1.235 | 1.193 | 1.221 | 1.206 | 1.193 |
| 4 | 1.356 | 1.292 | 1.323 | 1.297 | 1.292 |
| 5 | 1.453 | 1.368 | 1.386 | 1.367 | 1.367 |
| 6 | 1.549 | 1.442 | 1.448 | 1.435 | 1.435 |
| 7 | 1.635 | 1.508 | 1.494 | 1.491 | 1.491 |
| 8 | 1.644 | 1.554 | 1.533 | 1.530 | 1.530 |
| 9 | 1.660 | 1.568 | 1.576 | 1.563 | 1.563 |
| 10 | 1.682 | 1.588 | 1.609 | 1.591 | 1.588 |
| 11 | 1.716 | 1.618 | 1.640 | 1.621 | 1.618 |
| 12 | 1.741 | 1.650 | 1.665 | 1.648 | 1.648 |
| 13 | 1.770 | 1.688 | 1.702 | 1.681 | 1.681 |
| 14 | 1.805 | 1.713 | 1.741 | 1.713 | 1.713 |
| 15 | 1.845 | 1.744 | 1.784 | 1.747 | 1.744 |
| 16 | 1.890 | 1.780 | 1.830 | 1.783 | 1.780 |
| 17 | 1.935 | 1.816 | 1.877 | 1.820 | 1.816 |
| 18 | 1.981 | 1.851 | 1.921 | 1.856 | 1.851 |
| 19 | 2.030 | 1.890 | 1.966 | 1.895 | 1.890 |
| 20 | 2.081 | 1.932 | 2.013 | 1.937 | 1.932 |
| 21 | 2.138 | 1.978 | 2.062 | 1.982 | 1.978 |
| 22 | 2.196 | 2.024 | 2.111 | 2.029 | 2.024 |
| 23 | 2.257 | 2.068 | 2.162 | 2.075 | 2.068 |
| 24 | 2.317 | 2.112 | 2.209 | 2.118 | 2.112 |
| 25 | 2.378 | 2.155 | 2.255 | 2.159 | 2.155 |
| 26 | 2.440 | 2.197 | 2.297 | 2.200 | 2.197 |
| 27 | 2.502 | 2.242 | 2.339 | 2.243 | 2.242 |
| 28 | 2.552 | 2.285 | 2.378 | 2.284 | 2.284 |
| 29 | 2.587 | 2.327 | 2.415 | 2.321 | 2.321 |
| 30 | 2.619 | 2.369 | 2.444 | 2.358 | 2.358 |
| 31 | 2.639 | 2.408 | 2.470 | 2.392 | 2.392 |
| 32 | 2.659 | 2.442 | 2.494 | 2.421 | 2.421 |
| 33 | 2.678 | 2.463 | 2.514 | 2.448 | 2.448 |
| 34 | 2.696 | 2.483 | 2.529 | 2.468 | 2.468 |
| 35 | 2.711 | 2.499 | 2.542 | 2.484 | 2.484 |
| 36 | 2.725 | 2.507 | 2.554 | 2.494 | 2.494 |
| 37 | 2.737 | 2.507 | 2.556 | 2.494 | 2.494 |
| 38 | 2.748 | 2.507 | 2.556 | 2.494 | 2.494 |
| 39 | 2.758 | 2.507 | 2.556 | 2.494 | 2.494 |
| 40 | 2.768 | 2.507 | 2.556 | 2.494 | 2.494 |
| 41 | 2.776 | 2.507 | 2.556 | 2.494 | 2.494 |
| 42 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 43 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 44 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 45 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 46 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 47 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 48 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 49 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 50 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 51 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |
| 52 | 2.784 | 2.507 | 2.556 | 2.494 | 2.494 |

### gaussian_mi / synthetic_gp_n100 (n=100, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.005 | 1.003 | 1.004 | 1.004 | 1.003 |
| 3 | 1.008 | 1.006 | 1.007 | 1.007 | 1.006 |
| 4 | 1.012 | 1.009 | 1.010 | 1.010 | 1.009 |
| 5 | 1.016 | 1.013 | 1.015 | 1.014 | 1.013 |
| 6 | 1.021 | 1.018 | 1.020 | 1.019 | 1.018 |
| 7 | 1.027 | 1.024 | 1.026 | 1.025 | 1.024 |
| 8 | 1.035 | 1.031 | 1.034 | 1.032 | 1.031 |
| 9 | 1.043 | 1.038 | 1.041 | 1.039 | 1.038 |
| 10 | 1.052 | 1.047 | 1.051 | 1.048 | 1.047 |
| 11 | 1.062 | 1.056 | 1.060 | 1.057 | 1.056 |
| 12 | 1.072 | 1.066 | 1.070 | 1.067 | 1.066 |
| 13 | 1.085 | 1.077 | 1.082 | 1.078 | 1.077 |
| 14 | 1.099 | 1.091 | 1.097 | 1.092 | 1.091 |
| 15 | 1.114 | 1.106 | 1.112 | 1.106 | 1.106 |
| 16 | 1.130 | 1.121 | 1.128 | 1.122 | 1.121 |
| 17 | 1.147 | 1.137 | 1.144 | 1.137 | 1.137 |
| 18 | 1.164 | 1.152 | 1.161 | 1.153 | 1.152 |
| 19 | 1.181 | 1.170 | 1.178 | 1.170 | 1.170 |
| 20 | 1.201 | 1.188 | 1.197 | 1.189 | 1.188 |
| 21 | 1.220 | 1.207 | 1.216 | 1.207 | 1.207 |
| 22 | 1.242 | 1.227 | 1.238 | 1.228 | 1.227 |
| 23 | 1.264 | 1.248 | 1.260 | 1.249 | 1.248 |
| 24 | 1.287 | 1.270 | 1.282 | 1.271 | 1.270 |
| 25 | 1.310 | 1.292 | 1.305 | 1.293 | 1.292 |
| 26 | 1.334 | 1.316 | 1.329 | 1.316 | 1.316 |
| 27 | 1.360 | 1.339 | 1.354 | 1.340 | 1.339 |
| 28 | 1.385 | 1.363 | 1.378 | 1.364 | 1.363 |
| 29 | 1.411 | 1.389 | 1.404 | 1.389 | 1.389 |
| 30 | 1.438 | 1.414 | 1.430 | 1.415 | 1.414 |
| 31 | 1.465 | 1.440 | 1.457 | 1.440 | 1.440 |
| 32 | 1.494 | 1.468 | 1.486 | 1.468 | 1.468 |
| 33 | 1.523 | 1.495 | 1.514 | 1.496 | 1.495 |
| 34 | 1.553 | 1.523 | 1.544 | 1.524 | 1.523 |
| 35 | 1.584 | 1.553 | 1.574 | 1.553 | 1.553 |
| 36 | 1.616 | 1.583 | 1.605 | 1.583 | 1.583 |
| 37 | 1.649 | 1.614 | 1.636 | 1.614 | 1.614 |
| 38 | 1.682 | 1.645 | 1.668 | 1.645 | 1.644 |
| 39 | 1.716 | 1.677 | 1.701 | 1.677 | 1.676 |
| 40 | 1.751 | 1.710 | 1.734 | 1.709 | 1.709 |
| 41 | 1.787 | 1.744 | 1.768 | 1.742 | 1.742 |
| 42 | 1.824 | 1.778 | 1.802 | 1.776 | 1.776 |
| 43 | 1.861 | 1.813 | 1.837 | 1.810 | 1.810 |
| 44 | 1.898 | 1.848 | 1.871 | 1.844 | 1.844 |
| 45 | 1.936 | 1.884 | 1.906 | 1.877 | 1.877 |
| 46 | 1.975 | 1.919 | 1.940 | 1.911 | 1.911 |
| 47 | 2.014 | 1.955 | 1.974 | 1.944 | 1.944 |
| 48 | 2.053 | 1.991 | 2.007 | 1.976 | 1.976 |
| 49 | 2.093 | 2.028 | 2.040 | 2.008 | 2.008 |
| 50 | 2.133 | 2.064 | 2.072 | 2.039 | 2.039 |

### gaussian_mi / synthetic_gp_n20 (n=20, ref = OPT)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.097 | 1.025 | 1.046 | 1.033 | 1.023 |
| 3 | 1.132 | 1.058 | 1.076 | 1.064 | 1.052 |
| 4 | 1.182 | 1.097 | 1.112 | 1.099 | 1.089 |
| 5 | 1.260 | 1.159 | 1.166 | 1.156 | 1.146 |
| 6 | 1.340 | 1.232 | 1.217 | 1.209 | 1.206 |
| 7 | 1.437 | 1.316 | 1.274 | 1.269 | 1.269 |
| 8 | 1.545 | 1.407 | 1.332 | 1.329 | 1.329 |
| 9 | 1.672 | 1.513 | 1.386 | 1.385 | 1.385 |
| 10 | 1.799 | 1.620 | 1.422 | 1.421 | 1.421 |
| 11 | 1.901 | 1.711 | 1.437 | 1.437 | 1.437 |
| 12 | 1.976 | 1.795 | 1.441 | 1.441 | 1.441 |
| 13 | 2.012 | 1.865 | 1.441 | 1.441 | 1.441 |
| 14 | 2.020 | 1.924 | 1.441 | 1.441 | 1.441 |
| 15 | 2.021 | 1.950 | 1.441 | 1.441 | 1.441 |
| 16 | 2.021 | 1.963 | 1.441 | 1.441 | 1.441 |
| 17 | 2.021 | 1.965 | 1.441 | 1.441 | 1.441 |
| 18 | 2.021 | 1.965 | 1.441 | 1.441 | 1.441 |
| 19 | 2.021 | 1.965 | 1.441 | 1.441 | 1.441 |
| 20 | 2.021 | 1.965 | 1.441 | 1.441 | 1.441 |

### hypergraph_cut / mag10_top1000 (n=1000, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 10 | 1.049 | 1.030 | 1.040 | 1.037 | 1.030 |
| 20 | 1.066 | 1.040 | 1.050 | 1.046 | 1.040 |
| 30 | 1.077 | 1.048 | 1.056 | 1.051 | 1.048 |
| 40 | 1.092 | 1.060 | 1.070 | 1.063 | 1.060 |
| 50 | 1.118 | 1.074 | 1.087 | 1.076 | 1.074 |
| 60 | 1.145 | 1.092 | 1.107 | 1.093 | 1.092 |
| 70 | 1.171 | 1.105 | 1.129 | 1.105 | 1.105 |
| 80 | 1.197 | 1.115 | 1.144 | 1.115 | 1.115 |
| 90 | 1.217 | 1.121 | 1.154 | 1.121 | 1.121 |
| 100 | 1.234 | 1.126 | 1.163 | 1.126 | 1.126 |
| 110 | 1.252 | 1.132 | 1.173 | 1.131 | 1.131 |
| 120 | 1.270 | 1.140 | 1.183 | 1.139 | 1.139 |
| 130 | 1.287 | 1.149 | 1.192 | 1.147 | 1.147 |
| 140 | 1.304 | 1.156 | 1.200 | 1.154 | 1.154 |
| 150 | 1.321 | 1.167 | 1.209 | 1.164 | 1.164 |
| 160 | 1.337 | 1.177 | 1.218 | 1.174 | 1.174 |
| 170 | 1.355 | 1.189 | 1.228 | 1.185 | 1.185 |
| 180 | 1.372 | 1.201 | 1.239 | 1.196 | 1.196 |
| 190 | 1.390 | 1.213 | 1.250 | 1.207 | 1.207 |
| 200 | 1.407 | 1.227 | 1.261 | 1.219 | 1.219 |
| 210 | 1.426 | 1.240 | 1.273 | 1.232 | 1.232 |
| 220 | 1.445 | 1.255 | 1.284 | 1.244 | 1.244 |
| 230 | 1.462 | 1.269 | 1.294 | 1.257 | 1.257 |
| 240 | 1.481 | 1.283 | 1.305 | 1.270 | 1.270 |
| 250 | 1.501 | 1.299 | 1.316 | 1.284 | 1.284 |
| 260 | 1.520 | 1.314 | 1.327 | 1.297 | 1.297 |
| 270 | 1.541 | 1.331 | 1.338 | 1.311 | 1.311 |
| 280 | 1.560 | 1.346 | 1.348 | 1.325 | 1.325 |
| 290 | 1.580 | 1.362 | 1.359 | 1.338 | 1.338 |
| 300 | 1.600 | 1.379 | 1.370 | 1.352 | 1.352 |

### hypergraph_cut / synthetic_n20 (n=20, ref = OPT)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 2 | 1.104 | 1.060 | 1.086 | 1.081 | 1.056 |
| 3 | 1.200 | 1.123 | 1.164 | 1.150 | 1.120 |
| 4 | 1.320 | 1.195 | 1.258 | 1.221 | 1.193 |
| 5 | 1.437 | 1.278 | 1.353 | 1.300 | 1.277 |
| 6 | 1.565 | 1.378 | 1.455 | 1.391 | 1.377 |
| 7 | 1.700 | 1.489 | 1.558 | 1.491 | 1.483 |
| 8 | 1.852 | 1.614 | 1.672 | 1.604 | 1.602 |
| 9 | 2.009 | 1.751 | 1.787 | 1.727 | 1.725 |
| 10 | 2.132 | 1.891 | 1.883 | 1.843 | 1.843 |
| 11 | 2.207 | 1.991 | 1.954 | 1.933 | 1.933 |
| 12 | 2.232 | 2.065 | 1.992 | 1.983 | 1.983 |
| 13 | 2.248 | 2.100 | 2.004 | 1.999 | 1.999 |
| 14 | 2.253 | 2.116 | 2.005 | 2.000 | 2.000 |
| 15 | 2.255 | 2.118 | 2.005 | 2.000 | 2.000 |
| 16 | 2.255 | 2.119 | 2.005 | 2.000 | 2.000 |
| 17 | 2.255 | 2.119 | 2.005 | 2.000 | 2.000 |
| 18 | 2.255 | 2.119 | 2.005 | 2.000 | 2.000 |
| 19 | 2.255 | 2.119 | 2.005 | 2.000 | 2.000 |
| 20 | 2.255 | 2.119 | 2.005 | 2.000 | 2.000 |

### hypergraph_cut / synthetic_n500 (n=500, ref = best found)

| k | Marginal | NM-Dual | B2 | Hybrid | min(NM-Dual, Hybrid) |
|---|---|---|---|---|---|
| 10 | 1.028 | 1.017 | 1.025 | 1.024 | 1.017 |
| 20 | 1.059 | 1.049 | 1.055 | 1.053 | 1.049 |
| 30 | 1.098 | 1.085 | 1.091 | 1.087 | 1.085 |
| 40 | 1.135 | 1.117 | 1.125 | 1.119 | 1.117 |
| 50 | 1.173 | 1.149 | 1.159 | 1.150 | 1.149 |
| 60 | 1.213 | 1.180 | 1.196 | 1.181 | 1.180 |
| 70 | 1.254 | 1.210 | 1.233 | 1.210 | 1.209 |
| 80 | 1.297 | 1.239 | 1.271 | 1.240 | 1.239 |
| 90 | 1.344 | 1.271 | 1.312 | 1.271 | 1.271 |
| 100 | 1.392 | 1.304 | 1.355 | 1.304 | 1.304 |
| 110 | 1.441 | 1.339 | 1.398 | 1.338 | 1.338 |
| 120 | 1.493 | 1.377 | 1.443 | 1.374 | 1.374 |
| 130 | 1.546 | 1.416 | 1.488 | 1.413 | 1.413 |
| 140 | 1.600 | 1.458 | 1.533 | 1.453 | 1.453 |
| 150 | 1.657 | 1.502 | 1.579 | 1.496 | 1.496 |
| 160 | 1.716 | 1.548 | 1.626 | 1.540 | 1.540 |
| 170 | 1.776 | 1.596 | 1.673 | 1.587 | 1.587 |
| 180 | 1.838 | 1.645 | 1.720 | 1.634 | 1.634 |
| 190 | 1.902 | 1.696 | 1.768 | 1.683 | 1.683 |
| 200 | 1.969 | 1.750 | 1.816 | 1.734 | 1.734 |
| 210 | 2.037 | 1.805 | 1.863 | 1.787 | 1.787 |
| 220 | 2.107 | 1.862 | 1.911 | 1.840 | 1.840 |
| 230 | 2.176 | 1.922 | 1.958 | 1.895 | 1.895 |
| 240 | 2.243 | 1.982 | 2.005 | 1.951 | 1.951 |
| 250 | 2.303 | 2.041 | 2.047 | 2.004 | 2.004 |

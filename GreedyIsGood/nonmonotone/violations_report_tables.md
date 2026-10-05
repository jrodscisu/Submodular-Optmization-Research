## 1. Controls and monotone-instance sanity checks

| problem | rows | monotone (instance, seed) | Top-k viol. | Marginal viol. | NM-Dual viol. | B2 viol. | Hybrid viol. | M1–M3 viol. on monotone seeds |
|---|---|---|---|---|---|---|---|---|
| max_cut | 200 | 0/10 | 0 | 0 | 0 | 0 | 0 | 0 |
| directed_cut | 352 | 0/17 | 0 | 0 | 0 | 0 | 0 | 0 |
| revenue_max | 431 | 0/21 | 0 | 0 | 0 | 0 | 0 | 0 |
| diverse_rec | 556 | 0/26 | 0 | 0 | 0 | 0 | 0 | 0 |
| log_det | 784 | 0/24 | 0 | 0 | 0 | 0 | 0 | 0 |
| gaussian_mi | 502 | 0/16 | 0 | 0 | 0 | 0 | 0 | 0 |
| hypergraph_cut | 361 | 0/16 | 0 | 0 | 0 | 0 | 0 | 0 |
| diverse_rec λ-sweep | 1400 | 40/70 | 0 | 0 | 0 | 0 | 0 | 0 |

### M3 solves

| problem | instance | rows | M3 literal | literal infeasible rows | M3 favorable | favorable infeasible rows |
|---|---|---|---|---|---|---|
| max_cut | k_sweep_n20_p0.3 | 200 | solved (0.0 s); solved (0.1 s) | 200 | solved (0.0 s); solved (0.1 s) | 0 |
| directed_cut | email_eu_core | 26 | solved (125.3 s) | 26 | skipped: > 300 s of solves | 0 |
| directed_cut | synthetic_n20 | 200 | solved (0.0 s) | 200 | solved (0.0 s); solved (0.1 s) | 0 |
| directed_cut | synthetic_n200 | 105 | solved (2.0 s); solved (2.2 s); solved (2.3 s); solved (2.5 s) | 105 | solved (7.7 s); solved (7.8 s); solved (7.9 s); solved (8.3 s) | 0 |
| directed_cut | wiki_vote | 21 | solved (332.4 s) | 21 | skipped: > 300 s of solves | 0 |
| revenue_max | facebook | 63 | skipped: B5 of the same size took 691 s; skipped: B5 of the same size took 734 s; skipped: B5 of the same size took 784 s | 0 | skipped: B5 of the same size took 691 s; skipped: B5 of the same size took 734 s; skipped: B5 of the same size took 784 s | 0 |
| revenue_max | synthetic_n20 | 200 | solved (0.0 s) | 200 | solved (0.0 s); solved (0.1 s) | 0 |
| revenue_max | synthetic_n200 | 105 | solved (0.7 s); solved (0.8 s) | 105 | solved (1.4 s); solved (1.7 s); solved (1.9 s); solved (2.1 s) | 0 |
| revenue_max | youtube | 63 | solved (104.8 s); solved (123.8 s); solved (125.0 s) | 63 | solved (93.1 s); solved (94.2 s); solved (95.8 s) | 0 |
| diverse_rec | ml20_lambda0.75 | 200 | solved (0.0 s); solved (0.1 s) | 140 | solved (0.0 s); solved (0.1 s) | 0 |
| diverse_rec | ml20_lambda1.0 | 200 | solved (0.0 s) | 200 | solved (0.0 s); solved (0.1 s) | 0 |
| diverse_rec | ml500_lambda0.75 | 78 | skipped: > 300 s of solves | 10 | skipped: > 300 s of solves | 0 |
| diverse_rec | ml500_lambda1.0 | 78 | solved (10.2 s); solved (18.0 s); solved (8.7 s) | 78 | skipped: > 300 s of solves; solved (297.6 s); solved (298.5 s) | 0 |
| log_det | synthetic_n20_alpha3 | 200 | solved (0.0 s) | 200 | solved (0.0 s) | 0 |
| log_det | wine_n100_alpha10 | 250 | solved (0.2 s); solved (0.3 s); solved (0.4 s) | 250 | solved (0.6 s); solved (0.7 s); solved (0.8 s) | 0 |
| log_det | wine_n100_alpha3 | 250 | solved (0.1 s); solved (0.2 s); solved (0.3 s) | 250 | solved (0.3 s); solved (0.4 s) | 0 |
| log_det | wine_n300_alpha10 | 42 | solved (0.1 s) | 42 | solved (0.3 s) | 0 |
| log_det | wine_n300_alpha3 | 42 | solved (0.1 s) | 42 | solved (0.2 s) | 0 |
| gaussian_mi | intel | 52 | solved (0.2 s) | 52 | solved (0.3 s) | 0 |
| gaussian_mi | synthetic_gp_n100 | 250 | solved (0.6 s); solved (0.7 s) | 250 | solved (1.9 s); solved (2.1 s); solved (2.3 s); solved (2.4 s) | 0 |
| gaussian_mi | synthetic_gp_n20 | 200 | solved (0.0 s) | 200 | solved (0.0 s); solved (0.1 s) | 0 |
| hypergraph_cut | mag10_top1000 | 31 | solved (409.2 s) | 31 | skipped: > 300 s of solves | 0 |
| hypergraph_cut | synthetic_n20 | 200 | solved (0.0 s) | 200 | solved (0.0 s) | 0 |
| hypergraph_cut | synthetic_n500 | 130 | skipped: B5 of the same size took 306 s; solved (23.7 s); solved (26.2 s); solved (26.5 s); solved (27.2 s) | 104 | skipped: B5 of the same size took 306 s; solved (243.4 s); solved (274.7 s); solved (279.2 s); solved (288.0 s) | 0 |

## 2. Violation rates per method and problem

*OPT rows*: rows where brute force gave OPT_k (rate = bound < OPT_k). *Certified rows*: the other rows, where a violation is certified only when the bound is below the best solution found (a lower bound on the true rate). Rates are over the rows where the method produced a bound (M3-literal: feasible LP). Severity = (ref − bound)/ref over violated rows. *Misleading* = an algorithm's own value exceeds the bound (greedy / random-greedy mean / best found), over all rows with a bound.

| method | problem | rows | OPT rows: rate | certified rows: rate | mean severity | max severity | misleading greedy | misleading RG mean | misleading best |
|---|---|---|---|---|---|---|---|---|---|
| M1 BQS Dual | max_cut | 200 | 81.0% (200) | – (0) | 0.272 | 0.417 | 79.0% | 69.0% | 81.0% |
| M1 BQS Dual | directed_cut | 352 | 70.0% (207) | 44.1% (145) | 0.137 | 0.329 | 58.8% | 8.8% | 59.4% |
| M1 BQS Dual | revenue_max | 431 | 82.0% (211) | 56.4% (220) | 0.132 | 0.217 | 68.9% | 47.3% | 68.9% |
| M1 BQS Dual | diverse_rec | 556 | 54.9% (406) | 13.3% (150) | 0.028 | 0.050 | 43.2% | 32.2% | 43.7% |
| M1 BQS Dual | log_det | 784 | 84.2% (234) | 96.9% (550) | 0.648 | 0.922 | 93.0% | 93.0% | 93.1% |
| M1 BQS Dual | gaussian_mi | 502 | 83.6% (220) | 74.5% (282) | 0.329 | 0.530 | 78.5% | 76.7% | 78.5% |
| M1 BQS Dual | hypergraph_cut | 361 | 77.7% (206) | 58.1% (155) | 0.165 | 0.344 | 69.0% | 60.9% | 69.3% |
| M2 Marginal (no pen.) | max_cut | 200 | 44.5% (200) | – (0) | 0.054 | 0.134 | 0.0% | 0.0% | 44.5% |
| M2 Marginal (no pen.) | directed_cut | 352 | 29.5% (207) | 0.0% (145) | 0.048 | 0.124 | 0.0% | 0.0% | 17.3% |
| M2 Marginal (no pen.) | revenue_max | 431 | 63.5% (211) | 0.0% (220) | 0.027 | 0.062 | 0.0% | 0.0% | 31.1% |
| M2 Marginal (no pen.) | diverse_rec | 556 | 48.0% (406) | 6.0% (150) | 0.006 | 0.015 | 0.0% | 1.6% | 36.7% |
| M2 Marginal (no pen.) | log_det | 784 | 63.7% (234) | 1.8% (550) | 0.053 | 0.129 | 0.0% | 0.0% | 20.3% |
| M2 Marginal (no pen.) | gaussian_mi | 502 | 51.8% (220) | 0.0% (282) | 0.015 | 0.028 | 0.0% | 0.0% | 22.7% |
| M2 Marginal (no pen.) | hypergraph_cut | 361 | 56.8% (206) | 0.0% (155) | 0.027 | 0.068 | 0.0% | 0.0% | 32.4% |
| M3 literal | max_cut | 0 | – (0) | – (0) | – | – | – | – | – |
| M3 literal | directed_cut | 0 | – (0) | – (0) | – | – | – | – | – |
| M3 literal | revenue_max | 0 | – (0) | – (0) | – | – | – | – | – |
| M3 literal | diverse_rec | 60 | 0.0% (60) | – (0) | – | – | 0.0% | 0.0% | 0.0% |
| M3 literal | log_det | 0 | – (0) | – (0) | – | – | – | – | – |
| M3 literal | gaussian_mi | 0 | – (0) | – (0) | – | – | – | – | – |
| M3 literal | hypergraph_cut | 0 | – (0) | – (0) | – | – | – | – | – |
| M3 favorable | max_cut | 200 | 100.0% (200) | – (0) | 1.000 | 1.000 | 100.0% | 100.0% | 100.0% |
| M3 favorable | directed_cut | 305 | 100.0% (205) | 100.0% (100) | 1.000 | 1.000 | 100.0% | 100.0% | 100.0% |
| M3 favorable | revenue_max | 368 | 100.0% (208) | 100.0% (160) | 1.000 | 1.000 | 100.0% | 100.0% | 100.0% |
| M3 favorable | diverse_rec | 452 | 85.1% (402) | 100.0% (50) | 0.713 | 1.000 | 86.7% | 86.7% | 86.7% |
| M3 favorable | log_det | 784 | 100.0% (234) | 100.0% (550) | 22.257 | 897.574 | 100.0% | 100.0% | 100.0% |
| M3 favorable | gaussian_mi | 502 | 100.0% (220) | 100.0% (282) | 1.000 | 1.000 | 100.0% | 100.0% | 100.0% |
| M3 favorable | hypergraph_cut | 304 | 100.0% (204) | 100.0% (100) | 1.000 | 1.000 | 100.0% | 100.0% | 100.0% |
| A1 caps, no pen. | max_cut | 200 | 45.0% (200) | – (0) | 0.054 | 0.134 | 0.0% | 0.0% | 45.0% |
| A1 caps, no pen. | directed_cut | 352 | 29.5% (207) | 0.0% (145) | 0.049 | 0.124 | 0.0% | 0.0% | 17.3% |
| A1 caps, no pen. | revenue_max | 431 | 63.5% (211) | 0.0% (220) | 0.027 | 0.062 | 0.0% | 0.0% | 31.1% |
| A1 caps, no pen. | diverse_rec | 556 | 48.0% (406) | 6.0% (150) | 0.006 | 0.015 | 0.0% | 1.6% | 36.7% |
| A1 caps, no pen. | log_det | 784 | 63.7% (234) | 1.8% (550) | 0.053 | 0.129 | 0.0% | 0.0% | 20.3% |
| A1 caps, no pen. | gaussian_mi | 502 | 52.3% (220) | 0.0% (282) | 0.015 | 0.028 | 0.0% | 0.0% | 22.9% |
| A1 caps, no pen. | hypergraph_cut | 361 | 56.8% (206) | 0.0% (155) | 0.027 | 0.068 | 0.0% | 0.0% | 32.4% |
| A2 pen., raw caps | max_cut | 200 | 80.0% (200) | – (0) | 0.259 | 0.417 | 78.5% | 60.5% | 80.0% |
| A2 pen., raw caps | directed_cut | 352 | 68.6% (207) | 44.1% (145) | 0.126 | 0.329 | 57.7% | 8.8% | 58.5% |
| A2 pen., raw caps | revenue_max | 431 | 81.5% (211) | 55.5% (220) | 0.119 | 0.217 | 68.2% | 39.2% | 68.2% |
| A2 pen., raw caps | diverse_rec | 556 | 54.7% (406) | 13.3% (150) | 0.026 | 0.050 | 43.0% | 30.9% | 43.5% |
| A2 pen., raw caps | log_det | 784 | 83.8% (234) | 96.9% (550) | 0.647 | 0.922 | 93.0% | 93.0% | 93.0% |
| A2 pen., raw caps | gaussian_mi | 502 | 83.6% (220) | 70.9% (282) | 0.313 | 0.530 | 76.5% | 74.3% | 76.5% |
| A2 pen., raw caps | hypergraph_cut | 361 | 77.7% (206) | 58.1% (155) | 0.161 | 0.344 | 68.7% | 57.1% | 69.3% |

## 3. Where violations occur in k (n = 20 instances, rows with OPT)

Violation rate per k range, and the smallest k with any violation.

| problem | method | k=1 | k=2–5 | k=6–10 | k=11–20 | first k |
|---|---|---|---|---|---|---|
| max_cut | M1 BQS Dual | 0.0% | 30.0% | 100.0% | 100.0% | 4 |
| max_cut | M2 Marginal (no pen.) | 0.0% | 0.0% | 38.0% | 70.0% | 8 |
| max_cut | M3 literal | – | – | – | – | – |
| max_cut | M3 favorable | 100.0% | 100.0% | 100.0% | 100.0% | 1 |
| max_cut | A1 caps, no pen. | 0.0% | 0.0% | 40.0% | 70.0% | 7 |
| max_cut | A2 pen., raw caps | 0.0% | 25.0% | 100.0% | 100.0% | 4 |
| directed_cut | M1 BQS Dual | 0.0% | 2.5% | 88.0% | 100.0% | 5 |
| directed_cut | M2 Marginal (no pen.) | 0.0% | 0.0% | 22.0% | 50.0% | 8 |
| directed_cut | M3 literal | – | – | – | – | – |
| directed_cut | M3 favorable | 100.0% | 100.0% | 100.0% | 100.0% | 1 |
| directed_cut | A1 caps, no pen. | 0.0% | 0.0% | 22.0% | 50.0% | 8 |
| directed_cut | A2 pen., raw caps | 0.0% | 2.5% | 82.0% | 100.0% | 5 |
| revenue_max | M1 BQS Dual | 0.0% | 57.5% | 100.0% | 100.0% | 3 |
| revenue_max | M2 Marginal (no pen.) | 0.0% | 2.5% | 86.0% | 90.0% | 5 |
| revenue_max | M3 literal | – | – | – | – | – |
| revenue_max | M3 favorable | 100.0% | 100.0% | 100.0% | 100.0% | 1 |
| revenue_max | A1 caps, no pen. | 0.0% | 2.5% | 86.0% | 90.0% | 5 |
| revenue_max | A2 pen., raw caps | 0.0% | 55.0% | 100.0% | 100.0% | 3 |
| diverse_rec | M1 BQS Dual | 0.0% | 0.0% | 30.0% | 96.5% | 8 |
| diverse_rec | M2 Marginal (no pen.) | 0.0% | 0.0% | 15.0% | 90.0% | 9 |
| diverse_rec | M3 literal | 0.0% | 0.0% | 0.0% | – | – |
| diverse_rec | M3 favorable | 50.0% | 50.0% | 90.0% | 100.0% | 1 |
| diverse_rec | A1 caps, no pen. | 0.0% | 0.0% | 15.0% | 90.0% | 9 |
| diverse_rec | A2 pen., raw caps | 0.0% | 0.0% | 29.0% | 96.5% | 8 |
| log_det | M1 BQS Dual | 0.0% | 100.0% | 100.0% | 100.0% | 2 |
| log_det | M2 Marginal (no pen.) | 0.0% | 35.0% | 90.0% | 90.0% | 4 |
| log_det | M3 literal | – | – | – | – | – |
| log_det | M3 favorable | 100.0% | 100.0% | 100.0% | 100.0% | 1 |
| log_det | A1 caps, no pen. | 0.0% | 35.0% | 90.0% | 90.0% | 4 |
| log_det | A2 pen., raw caps | 0.0% | 97.5% | 100.0% | 100.0% | 2 |
| gaussian_mi | M1 BQS Dual | 0.0% | 85.0% | 100.0% | 100.0% | 2 |
| gaussian_mi | M2 Marginal (no pen.) | 0.0% | 0.0% | 48.0% | 90.0% | 8 |
| gaussian_mi | M3 literal | – | – | – | – | – |
| gaussian_mi | M3 favorable | 100.0% | 100.0% | 100.0% | 100.0% | 1 |
| gaussian_mi | A1 caps, no pen. | 0.0% | 0.0% | 50.0% | 90.0% | 7 |
| gaussian_mi | A2 pen., raw caps | 0.0% | 85.0% | 100.0% | 100.0% | 2 |
| hypergraph_cut | M1 BQS Dual | 0.0% | 25.0% | 100.0% | 100.0% | 4 |
| hypergraph_cut | M2 Marginal (no pen.) | 0.0% | 0.0% | 54.0% | 90.0% | 7 |
| hypergraph_cut | M3 literal | – | – | – | – | – |
| hypergraph_cut | M3 favorable | 100.0% | 100.0% | 100.0% | 100.0% | 1 |
| hypergraph_cut | A1 caps, no pen. | 0.0% | 0.0% | 54.0% | 90.0% | 7 |
| hypergraph_cut | A2 pen., raw caps | 0.0% | 25.0% | 100.0% | 100.0% | 4 |

## 4. Ablation of our two fixes (all rows; OPT rows / certified rows)

M1 = neither fix (monotone caps, no penalty); A1 = our caps only; A2 = penalty only (raw caps); NM-Dual = both.

| problem | M1 (neither) | A1 (caps only) | A2 (penalty only) | NM-Dual (both) |
|---|---|---|---|---|
| max_cut | 81.0% / – | 45.0% / – | 80.0% / – | 0.0% / – |
| directed_cut | 70.0% / 44.1% | 29.5% / 0.0% | 68.6% / 44.1% | 0.0% / 0.0% |
| revenue_max | 82.0% / 56.4% | 63.5% / 0.0% | 81.5% / 55.5% | 0.0% / 0.0% |
| diverse_rec | 54.9% / 13.3% | 48.0% / 6.0% | 54.7% / 13.3% | 0.0% / 0.0% |
| log_det | 84.2% / 96.9% | 63.7% / 1.8% | 83.8% / 96.9% | 0.0% / 0.0% |
| gaussian_mi | 83.6% / 74.5% | 52.3% / 0.0% | 83.6% / 70.9% | 0.0% / 0.0% |
| hypergraph_cut | 77.7% / 58.1% | 56.8% / 0.0% | 77.7% / 58.1% | 0.0% / 0.0% |

## 5. λ-sweep (diverse rec, n = 20, 10 seeds, k = 1..20, OPT for every row)

| λ | monotone seeds | M1 BQS Dual | M2 Marginal (no pen.) | M3 literal | M3 favorable | A1 caps, no pen. | A2 pen., raw caps |
|---|---|---|---|---|---|---|---|
| 0.000 | 100.0% | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) |
| 0.100 | 100.0% | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) |
| 0.250 | 100.0% | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) |
| 0.500 | 100.0% | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) | 0.0% (sev 0.000) |
| 0.750 | 0.0% | 46.5% (sev 0.022) | 40.0% (sev 0.005) | 0.0% (sev 0.000) | 70.0% (sev 0.197) | 40.0% (sev 0.005) | 46.5% (sev 0.020) |
| 1.000 | 0.0% | 65.0% (sev 0.034) | 57.5% (sev 0.007) | – | 100.0% (sev 1.000) | 57.5% (sev 0.007) | 64.5% (sev 0.031) |
| 1.500 | 0.0% | 79.5% (sev 0.053) | 75.0% (sev 0.007) | – | 100.0% (sev 4.876) | 75.0% (sev 0.007) | 79.5% (sev 0.052) |

Cells: violation rate (mean severity over violated rows). M3-literal: over rows with a feasible LP; infeasible rows per λ: λ=0: 0, λ=0.1: 0, λ=0.25: 0, λ=0.5: 0, λ=0.75: 140, λ=1: 200, λ=1.5: 200.

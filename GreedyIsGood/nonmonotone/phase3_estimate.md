# Phase 3 estimate: untruncated base sets (seed 0 of each instance)

Extra NM-Dual time = new prefixes x mean cost of the last 5 measured prefixes (an over-estimate). The --baselines run recomputes the caps for B5, so its extra time is about twice this. Hybrid solve estimate = #k x (largest current LP time) x (new columns / current columns), assuming linear scaling; B2 grows the same way but its solves are far cheaper.

| problem | instance | n | k_max | stride | t (prefixes now) | t_full | new prefixes | NM-Dual now [s] | extra NM-Dual [s] | base sets at every k | max hybrid cols | hybrid solve now, all k [s] | hybrid solve est., all k [s] |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| src | maxcut_generic | 20 | 20 | 1 | 11 | 11 | 0 | 0.0 | 0.0 | 12 -> 12 | 0 -> 0 | 0.0 | 0.0 |
| directed_cut | synthetic_n20 | 20 | 20 | 1 | 9 | 9 | 0 | 0.0 | 0.0 | 10 -> 10 | 176 -> 176 | 0.0 | 0.1 |
| directed_cut | synthetic_n200 | 200 | 100 | 1 | 96 | 96 | 0 | 0.1 | 0.0 | 97 -> 97 | 14945 -> 14945 | 4.7 | 11.7 |
| directed_cut | email_eu_core | 1005 | 500 | 1 | 302 | 302 | 0 | 15.4 | 0.0 | 303 -> 303 | 259768 -> 259768 | 192.9 | 426.9 |
| directed_cut | wiki_vote | 7115 | 200 | 10 | 200 | 4756 | 456 | 52.7 | 847.2 | 22 -> 478 | 154431 -> 2265715 | 95.6 | 2692.0 |
| revenue_max | synthetic_n20 | 20 | 20 | 1 | 7 | 7 | 0 | 0.0 | 0.0 | 8 -> 8 | 153 -> 153 | 0.0 | 0.1 |
| revenue_max | synthetic_n200 | 200 | 100 | 1 | 64 | 64 | 0 | 0.1 | 0.0 | 65 -> 65 | 11121 -> 11121 | 5.4 | 15.3 |
| revenue_max | facebook | 4039 | 200 | 10 | 200 | 886 | 69 | 85.2 | 228.2 | 22 -> 91 | 86759 -> 327504 | 734.1 | 9098.1 |
| revenue_max | youtube | 3804 | 200 | 5 | 200 | 589 | 78 | 24.5 | 34.8 | 42 -> 120 | 155669 -> 421377 | 178.3 | 1415.2 |
| diverse_recommendation | ml20_lambda0.75 | 20 | 20 | 1 | 13 | 13 | 0 | 0.0 | 0.0 | 14 -> 14 | 210 -> 210 | 0.1 | 0.1 |
| diverse_recommendation | ml20_lambda1.0 | 20 | 20 | 1 | 10 | 10 | 0 | 0.0 | 0.0 | 11 -> 11 | 186 -> 186 | 0.0 | 0.1 |
| diverse_recommendation | ml500_lambda0.75 | 500 | 250 | 1 | 250 | 312 | 62 | 9.2 | 1.0 | 251 -> 313 | 94626 -> 108173 | 202.8 | 542.7 |
| diverse_recommendation | ml500_lambda1.0 | 500 | 250 | 1 | 228 | 228 | 0 | 8.6 | 0.0 | 229 -> 229 | 88895 -> 88895 | 132.7 | 336.4 |
| log_determinant | synthetic_n20_alpha3 | 20 | 20 | 1 | 5 | 5 | 0 | 0.0 | 0.0 | 6 -> 6 | 126 -> 126 | 0.0 | 0.1 |
| log_determinant | wine_n100_alpha3 | 100 | 50 | 1 | 15 | 15 | 0 | 1.3 | 0.0 | 16 -> 16 | 1581 -> 1581 | 0.5 | 1.1 |
| log_determinant | wine_n100_alpha10 | 100 | 50 | 1 | 32 | 32 | 0 | 2.9 | 0.0 | 33 -> 33 | 2873 -> 2873 | 1.5 | 2.7 |
| log_determinant | wine_n300_alpha3 | 300 | 100 | 5 | 23 | 23 | 0 | 56.5 | 0.0 | 7 -> 7 | 2028 -> 2028 | 0.4 | 0.5 |
| log_determinant | wine_n300_alpha10 | 300 | 100 | 5 | 52 | 52 | 0 | 101.1 | 0.0 | 13 -> 13 | 3574 -> 3574 | 1.0 | 1.6 |
| gaussian_mi | synthetic_gp_n20 | 20 | 20 | 1 | 9 | 9 | 0 | 0.0 | 0.0 | 10 -> 10 | 176 -> 176 | 0.0 | 0.2 |
| gaussian_mi | synthetic_gp_n100 | 100 | 50 | 1 | 50 | 52 | 2 | 4.9 | 0.1 | 51 -> 53 | 3926 -> 4023 | 1.8 | 4.0 |
| gaussian_mi | intel | 52 | 52 | 1 | 26 | 26 | 0 | 0.2 | 0.0 | 27 -> 27 | 1106 -> 1106 | 0.4 | 0.8 |
| hypergraph_cut | synthetic_n20 | 20 | 20 | 1 | 9 | 9 | 0 | 0.0 | 0.0 | 10 -> 10 | 176 -> 176 | 0.0 | 0.1 |
| hypergraph_cut | synthetic_n500 | 500 | 250 | 1 | 235 | 235 | 0 | 1.5 | 0.0 | 236 -> 236 | 90771 -> 90771 | 266.0 | 1045.8 |
| hypergraph_cut | mag10_top1000 | 1000 | 300 | 1 | 300 | 414 | 114 | 8.6 | 1.6 | 301 -> 415 | 256851 -> 330096 | 262.6 | 928.4 |

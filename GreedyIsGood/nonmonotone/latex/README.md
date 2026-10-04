# LaTeX figures: random greedy certified ratio vs k (pgfplots)

One plot per problem and dataset of RG / B for every upper bound B (dual, top-k singletons, total
weight, and OPT where brute-forced), with the 1/e line; RG is random
greedy's mean value. `main.pdf` is the compiled demo (two-column article).

```
latex/
├── make_pgfplots.py        regenerates data/, plots/, figures/ from the result CSVs (stdlib only)
├── rgplots-preamble.tex    \usepackage{pgfplots} + colors and styles
├── main.tex / main.pdf     demo document with every figure
├── data/<problem>__<instance>.dat     per-k means and sds over seeds (nan = not shown)
├── plots/<problem>__<instance>.tex    one tikzpicture per instance, width \rgplotwidth
└── figures/<problem>.tex              single-column figure (2 plots per row, shared legend)
    figures/<problem>_wide.tex         two-column figure* (4 per row)
```

Use in a paper (copy this folder to e.g. `figs/rg/`):
```latex
\newcommand{\rgroot}{figs/rg}            % path of this folder, relative to the main .tex
\input{figs/rg/rgplots-preamble}         % preamble
...
\input{figs/rg/figures/revenue_max}      % body; or revenue_max_wide for figure*
```
To place single plots yourself: `\setlength{\rgplotwidth}{0.49\columnwidth}` and
`\input{\rgroot/plots/revenue_max__facebook.tex}`. Requires pgfplots >= 1.18 (TeX Live 2021+).
Problems: `max_cut` (the original n = 20 sweep from `src/`), `directed_cut`, `revenue_max`,
`diverse_rec`, `log_det`, `gaussian_mi`, `hypergraph_cut`.

After new experiment runs: `python3 latex/make_pgfplots.py` (`--total-max-ratio R` would hide
the total-weight curve wherever the total bound exceeds R times the dual bound; default: never).

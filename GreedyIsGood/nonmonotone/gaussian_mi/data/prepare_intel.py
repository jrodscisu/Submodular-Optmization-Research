"""Covariance of the Intel Berkeley lab temperature sensors, for Gaussian mutual information.

Reads data.txt.gz ("date time epoch moteid temperature humidity light voltage"), keeps
readings from the first --days days (later on, failing batteries produce bogus readings) with
temperature in [--tmin, --tmax], averages each mote's temperature over --bin-minutes bins,
keeps motes present in >= --coverage of the bins and bins where all kept motes are present,
and writes the sample covariance to intel_temp_cov.txt ("n" then an n x n matrix) together
with the mote ids (intel_temp_motes.txt). Standard library only.
"""
import argparse
import gzip
from collections import defaultdict
from datetime import datetime

ap = argparse.ArgumentParser()
ap.add_argument("--days", type=float, default=8)
ap.add_argument("--bin-minutes", type=int, default=30)
ap.add_argument("--coverage", type=float, default=0.9)
ap.add_argument("--tmin", type=float, default=10)
ap.add_argument("--tmax", type=float, default=40)
args = ap.parse_args()

sums = defaultdict(lambda: [0.0, 0])  # (mote, bin) -> [sum, count]
with gzip.open("data.txt.gz", "rt") as f:
    for line in f:
        p = line.split()
        if len(p) < 5:
            continue
        try:
            ts = datetime.strptime(p[0] + " " + p[1][:8], "%Y-%m-%d %H:%M:%S")
            mote, temp = int(p[3]), float(p[4])
        except ValueError:
            continue
        hours = (ts - datetime(2004, 2, 28)).total_seconds() / 3600
        if hours < 0 or hours > 24 * args.days or not (args.tmin <= temp <= args.tmax):
            continue
        b = int(hours * 60 // args.bin_minutes)
        s = sums[(mote, b)]
        s[0] += temp
        s[1] += 1

bins = sorted({b for _, b in sums})
motes = sorted({m for m, _ in sums})
present = {m: sum((m, b) in sums for b in bins) for m in motes}
motes = [m for m in motes if present[m] >= args.coverage * len(bins)]
rows = [b for b in bins if all((m, b) in sums for m in motes)]
X = [[sums[(m, b)][0] / sums[(m, b)][1] for b in rows] for m in motes]

n, T = len(motes), len(rows)
mu = [sum(x) / T for x in X]
C = [[sum((X[i][t] - mu[i]) * (X[j][t] - mu[j]) for t in range(T)) / (T - 1) for j in range(n)] for i in range(n)]

with open("intel_temp_cov.txt", "w") as f:
    f.write(f"{n}\n")
    for row in C:
        f.write(" ".join(f"{v:.10g}" for v in row) + "\n")
with open("intel_temp_motes.txt", "w") as f:
    f.write(" ".join(map(str, motes)) + "\n")
print(f"wrote intel_temp_cov.txt: {n} motes, {T} time bins of {args.bin_minutes} min "
      f"(first {args.days:g} days; {len(bins)} bins before filtering)")

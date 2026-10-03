"""Community-based subgraph of SNAP com-YouTube (1.1M nodes is far beyond the O(n^2) oracle
calls per set that the dual bound needs).

Takes communities from com-youtube.top5000.cmty.txt.gz in file order, unions their members
until at least --nodes nodes are collected, keeps the induced subgraph's largest connected
component and writes it as an edge list youtube_cmty_<nodes>.txt (node ids relabeled 0..n-1).
Standard library only.
"""
import argparse
import gzip
from collections import defaultdict, deque

ap = argparse.ArgumentParser()
ap.add_argument("--nodes", type=int, default=3000)
args = ap.parse_args()

keep = set()
with gzip.open("com-youtube.top5000.cmty.txt.gz", "rt") as f:
    for line in f:
        keep.update(int(x) for x in line.split())
        if len(keep) >= args.nodes:
            break

adj = defaultdict(set)
with gzip.open("com-youtube.ungraph.txt.gz", "rt") as f:
    for line in f:
        if line.startswith("#"):
            continue
        u, v = map(int, line.split())
        if u in keep and v in keep and u != v:
            adj[u].add(v)
            adj[v].add(u)

# largest connected component
seen, best = set(), []
for s in adj:
    if s in seen:
        continue
    comp, q = [], deque([s])
    seen.add(s)
    while q:
        u = q.popleft()
        comp.append(u)
        for v in adj[u]:
            if v not in seen:
                seen.add(v)
                q.append(v)
    if len(comp) > len(best):
        best = comp

ids = {u: i for i, u in enumerate(sorted(best))}
out = f"youtube_cmty_{args.nodes}.txt"
m = 0
with open(out, "w") as f:
    f.write(f"# com-YouTube subgraph: union of top communities, LCC, {len(ids)} nodes\n")
    for u in sorted(best):
        for v in adj[u]:
            if u < v:
                f.write(f"{ids[u]} {ids[v]}\n")
                m += 1
print(f"wrote {out}: {len(ids)} nodes, {m} edges (from {len(keep)} community members)")

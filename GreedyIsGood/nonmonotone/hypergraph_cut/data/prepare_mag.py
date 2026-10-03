"""Co-authorship hypergraph from cat-edge-MAG-10 restricted to the most prolific authors.

Keeps the --authors authors with the most papers, restricts every paper (hyperedge) to them,
drops hyperedges with fewer than 2 remaining authors, merges identical author sets (weight =
number of papers) and writes mag10_top<authors>.txt: one hyperedge per line "w v1 v2 ...",
node ids 0..n-1. Standard library only.
"""
import argparse
from collections import Counter

ap = argparse.ArgumentParser()
ap.add_argument("--authors", type=int, default=1000)
args = ap.parse_args()

papers = []
with open("cat-edge-MAG-10/hyperedges.txt") as f:
    for line in f:
        e = [int(x) for x in line.replace(",", " ").split()]
        if e:
            papers.append(e)

deg = Counter(v for e in papers for v in set(e))
top = sorted(deg, key=lambda v: (-deg[v], v))[: args.authors]
ids = {v: i for i, v in enumerate(sorted(top))}

edges = Counter()
for e in papers:
    r = tuple(sorted({ids[v] for v in e if v in ids}))
    if len(r) >= 2:
        edges[r] += 1

used = {v for e in edges for v in e}
out = f"mag10_top{args.authors}.txt"
with open(out, "w") as f:
    f.write(f"# cat-edge-MAG-10, top {args.authors} authors by #papers; weight = #papers with that author set\n")
    for e, w in sorted(edges.items()):
        f.write(f"{w} " + " ".join(map(str, e)) + "\n")
sizes = Counter(len(e) for e in edges)
print(f"wrote {out}: {len(edges)} hyperedges over {len(used)} authors (of {args.authors}); "
      f"sizes {dict(sorted(sizes.items()))}")

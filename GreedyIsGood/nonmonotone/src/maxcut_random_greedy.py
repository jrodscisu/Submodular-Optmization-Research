"""Max-cut under a cardinality constraint via Random Greedy
(Buchbinder, Feldman, Naor, Schwartz 2014): (1/e)-approximation in expectation
for non-monotone submodular maximization s.t. |S| <= k.

f(S) = total weight of edges crossing (S, V \ S)  -- non-negative, submodular, non-monotone.
"""
import itertools
import numpy as np


def synthetic_graph(n, p, rng, weighted=True):
    """Erdos-Renyi G(n,p) as a symmetric weight matrix."""
    upper = np.triu(rng.random((n, n)) < p, 1)
    w = np.where(weighted, rng.integers(1, 10, (n, n)), 1) * upper
    return (w + w.T).astype(float)


def cut_value(W, S):
    mask = np.zeros(len(W), dtype=bool)
    mask[list(S)] = True
    return W[mask][:, ~mask].sum()


def random_greedy(W, k, rng):
    """Each round: take the k elements with largest marginal gain (padded with
    zero-gain dummies when fewer than k have positive gain), pick one uniformly."""
    n = len(W)
    in_S = np.zeros(n, dtype=bool)
    # gain(v | S) = w(v, V\S) - w(v, S) = deg(v) - 2 w(v, S)
    deg = W.sum(1)
    w_to_S = np.zeros(n)
    for _ in range(k):
        gains = deg - 2 * w_to_S
        gains[in_S] = -np.inf
        cand = np.argsort(-gains)[:k]
        cand = cand[gains[cand] > 0]          # non-positive gains <-> dummies
        j = rng.integers(k)                   # uniform over k slots (cand + dummies)
        if j < len(cand):
            v = cand[j]
            in_S[v] = True
            w_to_S += W[v]
    S = np.flatnonzero(in_S)
    return S, cut_value(W, S)


def plain_greedy(W, k):
    n = len(W)
    in_S = np.zeros(n, dtype=bool)
    deg = W.sum(1)
    w_to_S = np.zeros(n)
    for _ in range(k):
        gains = deg - 2 * w_to_S
        gains[in_S] = -np.inf
        v = int(np.argmax(gains))
        if gains[v] <= 0:
            break
        in_S[v] = True
        w_to_S += W[v]
    S = np.flatnonzero(in_S)
    return S, cut_value(W, S)


def brute_force(W, k):
    n = len(W)
    best, best_S = 0.0, ()
    for size in range(1, k + 1):
        for S in itertools.combinations(range(n), size):
            val = cut_value(W, S)
            if val > best:
                best, best_S = val, S
    return best_S, best


if __name__ == "__main__":
    rng = np.random.default_rng(0)
    n, p, k, trials = 20, 0.3, 6, 2000
    W = synthetic_graph(n, p, rng)
    print(f"G(n={n}, p={p}), edges={int((W > 0).sum() // 2)}, k={k}")

    opt_S, opt = brute_force(W, k)
    g_S, g_val = plain_greedy(W, k)
    vals = np.array([random_greedy(W, k, rng)[1] for _ in range(trials)])

    print(f"OPT (brute force)      : {opt:.1f}  S={sorted(opt_S)}")
    print(f"Plain greedy           : {g_val:.1f}  ratio={g_val / opt:.3f}")
    print(f"Random greedy ({trials} runs): mean={vals.mean():.1f}  "
          f"ratio={vals.mean() / opt:.3f}  min={vals.min():.1f}  max={vals.max():.1f}")
    print(f"1/e guarantee          : {opt / np.e:.1f}  "
          f"({'OK' if vals.mean() >= opt / np.e else 'VIOLATED'})")

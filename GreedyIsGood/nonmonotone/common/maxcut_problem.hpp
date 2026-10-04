// Max-cut as a problem type for the generic code, with the graph generator of
// src/maxcut_random_greedy.cpp (identical RNG use, so seed s gives the same graph).
// Used by common/port_check.cpp and src/maxcut_baselines.cpp.
#pragma once
#include <numeric>
#include <random>
#include <vector>

namespace dual {

using Matrix = std::vector<std::vector<double>>;

// identical to src/maxcut_random_greedy.cpp
inline Matrix synthetic_graph(int n, double p, std::mt19937_64& rng) {
    Matrix W(n, std::vector<double>(n, 0.0));
    std::uniform_real_distribution<double> U(0, 1);
    std::uniform_int_distribution<int> wt(1, 9);
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (U(rng) < p) W[i][j] = W[j][i] = wt(rng);
    return W;
}

struct MaxCut {
    Matrix W;
    int n() const { return W.size(); }
    double eval(const std::vector<char>& in_S) const {
        double c = 0;
        for (int i = 0; i < n(); i++)
            if (in_S[i])
                for (int j = 0; j < n(); j++)
                    if (!in_S[j]) c += W[i][j];
        return c;
    }
    struct State {
        const MaxCut* P;
        std::vector<double> deg, w_to_S;
        std::vector<char> in_S;
        explicit State(const MaxCut& p) : P(&p), deg(p.n()), w_to_S(p.n(), 0.0), in_S(p.n(), 0) {
            for (int i = 0; i < p.n(); i++) deg[i] = std::accumulate(p.W[i].begin(), p.W[i].end(), 0.0);
        }
        double gain(int v) const { return deg[v] - 2 * w_to_S[v]; }
        void add(int v) {
            in_S[v] = 1;
            for (int u = 0; u < P->n(); u++) w_to_S[u] += P->W[v][u];
        }
        void remove(int v) {
            in_S[v] = 0;
            for (int u = 0; u < P->n(); u++) w_to_S[u] -= P->W[v][u];
        }
    };
};

}  // namespace dual

// Directed cut under |S| <= k:  f(S) = sum of w(u -> v) over edges with u in S, v not in S.
// Non-negative, submodular, non-monotone.
//
//   gain(v | S) = f(S + v) - f(S - v) = w(v -> V \ S) - w(S -> v)       (no self-loops)
//              = outdeg(v) - w(v -> S) - w(S -> v),  maintained in O(1); add/remove O(deg v)
//   top-k bound: k largest out-degrees;  total bound: total edge weight
//
// Build: g++ -O2 -std=c++17 directed_cut.cpp -o directed_cut
// Instances:
//   --synthetic N --p P               G(N, P), weights 1..9, random orientation (seeded)
//   --graph FILE [--weights S]        directed edge list "u v [w]"; S = file|unit|uniform01|int1-9
// Sweep options (common/experiment.hpp): --ks 1:50:5 --trials 200 --seed 0 --out r.csv
//   --prefix-out p.csv --brute-budget 2e6 --chain-stride 1 --check-direct --instance NAME
#include "../common/experiment.hpp"
#include "../common/graph_io.hpp"

using namespace std;
using namespace dual;

struct DirectedCut {
    int N = 0;
    vector<vector<pair<int, double>>> out, in;
    vector<double> outdeg;
    double total = 0;

    explicit DirectedCut(const EdgeList& g) : N(g.n), out(g.n), in(g.n), outdeg(g.n, 0.0) {
        for (auto& e : g.edges) {
            out[e.u].push_back({e.v, e.w});
            in[e.v].push_back({e.u, e.w});
            outdeg[e.u] += e.w;
            total += e.w;
        }
    }
    int n() const { return N; }

    double eval(const vector<char>& S) const {
        double c = 0;
        for (int u = 0; u < N; u++)
            if (S[u])
                for (auto [v, w] : out[u])
                    if (!S[v]) c += w;
        return c;
    }

    struct State {
        const DirectedCut* P;
        vector<char> in_S;
        vector<double> out_to_S, in_from_S;  // w(u -> S), w(S -> u)
        explicit State(const DirectedCut& p)
            : P(&p), in_S(p.N, 0), out_to_S(p.N, 0.0), in_from_S(p.N, 0.0) {}
        double gain(int v) const { return P->outdeg[v] - out_to_S[v] - in_from_S[v]; }
        void add(int v) {
            in_S[v] = 1;
            for (auto [u, w] : P->in[v]) out_to_S[u] += w;   // edge u -> v
            for (auto [u, w] : P->out[v]) in_from_S[u] += w;  // edge v -> u
        }
        void remove(int v) {
            in_S[v] = 0;
            for (auto [u, w] : P->in[v]) out_to_S[u] -= w;
            for (auto [u, w] : P->out[v]) in_from_S[u] -= w;
        }
    };
};

int main(int argc, char** argv) {
    Args a(argc, argv);
    unsigned long long seed = a.integer("seed", 0);
    EdgeList g;
    string name;
    if (a.has("synthetic")) {
        int n = a.integer("synthetic", 20);
        double p = a.num("p", 0.3);
        g = gnp_graph(n, p, true, seed);
        name = "synthetic_n" + to_string(n) + "_p" + a.str("p", "0.3");
    } else if (a.has("graph")) {
        g = read_edge_list(a.str("graph"), true);
        reweight(g, a.str("weights", "file"), seed);
        name = a.str("graph");
        name = name.substr(name.find_last_of('/') + 1);
        name = name.substr(0, name.find('.'));
    } else {
        fprintf(stderr, "need --synthetic N or --graph FILE\n");
        return 1;
    }
    DirectedCut P(g);
    printf("directed cut: n=%d, edges=%zu, total weight=%.1f\n", P.n(), g.edges.size(), P.total);
    run_k_sweep(P, sweep_config(a, P.n(), "directed_cut", name, P.total));
}

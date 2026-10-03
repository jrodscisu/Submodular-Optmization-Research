// Revenue maximization (Mirzasoleiman, Badanidiyuru, Karbasi 2016) under |S| <= k:
//   f(S) = sum_{i not in S} sqrt( sum_{j in S} w_ij )        (undirected weighted graph)
// Non-negative, submodular, non-monotone (putting a node in S forfeits its own revenue).
//
// With a_i = sum_{j in S} w_ij maintained, for A = S - v:
//   gain(v | S) = f(A + v) - f(A) = -sqrt(a_v) + sum_{i ~ v, i not in S} [sqrt(a_i(A) + w_iv) - sqrt(a_i(A))]
// where a_i(A) = a_i - w_iv [v in S].  O(deg v) per gain, add and remove.
//   top-k bound: k largest singleton values f({v}) = sum_{i ~ v} sqrt(w_iv)
//   total bound: sum_i sqrt(sum_j w_ij)   (every term at its largest possible value)
//
// Build: g++ -O2 -std=c++17 revenue_max.cpp -o revenue_max
// Instances:
//   --synthetic N --p P               G(N, P) with weights U(0,1) (seeded)
//   --graph FILE [--weights S]        undirected edge list "u v [w]"; S = file|unit|uniform01|int1-9
// Sweep options: see common/experiment.hpp
#include "../common/experiment.hpp"
#include "../common/graph_io.hpp"

using namespace std;
using namespace dual;

struct RevenueMax {
    int N = 0;
    vector<vector<pair<int, double>>> adj;
    double total = 0;

    explicit RevenueMax(const EdgeList& g) : N(g.n), adj(g.n) {
        vector<double> wdeg(g.n, 0.0);
        for (auto& e : g.edges) {
            adj[e.u].push_back({e.v, e.w});
            adj[e.v].push_back({e.u, e.w});
            wdeg[e.u] += e.w, wdeg[e.v] += e.w;
        }
        for (double d : wdeg) total += sqrt(d);
    }
    int n() const { return N; }

    double eval(const vector<char>& S) const {
        double f = 0;
        for (int i = 0; i < N; i++) {
            if (S[i]) continue;
            double a = 0;
            for (auto [j, w] : adj[i]) if (S[j]) a += w;
            f += sqrt(a);
        }
        return f;
    }

    struct State {
        const RevenueMax* P;
        vector<char> in_S;
        vector<double> a;  // a_i = w(i, S)
        explicit State(const RevenueMax& p) : P(&p), in_S(p.N, 0), a(p.N, 0.0) {}
        double gain(int v) const {
            double g = -sqrt(max(0.0, a[v]));
            for (auto [i, w] : P->adj[v]) {
                if (in_S[i]) continue;
                double ai = max(0.0, in_S[v] ? a[i] - w : a[i]);
                g += sqrt(ai + w) - sqrt(ai);
            }
            return g;
        }
        void add(int v) {
            in_S[v] = 1;
            for (auto [i, w] : P->adj[v]) a[i] += w;
        }
        void remove(int v) {
            in_S[v] = 0;
            for (auto [i, w] : P->adj[v]) a[i] -= w;
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
        g = gnp_graph(n, a.num("p", 0.3), false, seed);
        reweight(g, "uniform01", seed);
        name = "synthetic_n" + to_string(n) + "_p" + a.str("p", "0.3");
    } else if (a.has("graph")) {
        g = read_edge_list(a.str("graph"), false);
        reweight(g, a.str("weights", "uniform01"), seed);
        name = a.str("graph");
        name = name.substr(name.find_last_of('/') + 1);
        name = name.substr(0, name.find('.'));
    } else {
        fprintf(stderr, "need --synthetic N or --graph FILE\n");
        return 1;
    }
    RevenueMax P(g);
    printf("revenue max: n=%d, edges=%zu, total bound=%.3f\n", P.n(), g.edges.size(), P.total);
    run_k_sweep(P, sweep_config(a, P.n(), "revenue_max", name, P.total));
}

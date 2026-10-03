// Hypergraph cut under |S| <= k:  f(S) = total weight of hyperedges e with 0 < |e ∩ S| < |e|.
// Non-negative, submodular, non-monotone (generalizes max-cut).
//
// With c_e = |e ∩ S| maintained, for A = S - v and c = c_e - [v in S]:
//   gain(v | S) = sum_{e ∋ v} w_e ( [c + 1 < |e|] - [c > 0] )       O(deg v); add/remove O(deg v)
//   top-k bound: k largest weighted degrees (singleton cut values);  total bound: sum_e w_e
//
// Build: g++ -O2 -std=c++17 hypergraph_cut.cpp -o hypergraph_cut
// Instances:
//   --hypergraph FILE        one hyperedge per line "w v1 v2 ... vs" (node ids 0..n-1; '#' comments)
//   --synthetic N --edges M [--min-size 2 --max-size 5]   uniform random hyperedges,
//                            weights 1..9 (seeded)
// Sweep options: see common/experiment.hpp
#include <fstream>

#include "../common/experiment.hpp"

using namespace std;
using namespace dual;

struct HypergraphCut {
    int N = 0;
    vector<vector<int>> edges;    // node lists
    vector<double> w;
    vector<vector<int>> incident;  // node -> hyperedge ids
    double total = 0;

    void finalize(int n) {
        N = n;
        incident.assign(N, {});
        for (size_t e = 0; e < edges.size(); e++) {
            for (int v : edges[e]) incident[v].push_back(e);
            total += w[e];
        }
    }
    int n() const { return N; }

    double eval(const vector<char>& S) const {
        double f = 0;
        for (size_t e = 0; e < edges.size(); e++) {
            int c = 0;
            for (int v : edges[e]) c += S[v];
            if (c > 0 && c < (int)edges[e].size()) f += w[e];
        }
        return f;
    }

    struct State {
        const HypergraphCut* P;
        vector<char> in_S;
        vector<int> cnt;  // |e ∩ S|
        explicit State(const HypergraphCut& p) : P(&p), in_S(p.N, 0), cnt(p.edges.size(), 0) {}
        double gain(int v) const {
            double g = 0;
            for (int e : P->incident[v]) {
                int c = cnt[e] - in_S[v], s = P->edges[e].size();
                g += P->w[e] * ((c + 1 < s) - (c > 0));
            }
            return g;
        }
        void add(int v) {
            in_S[v] = 1;
            for (int e : P->incident[v]) cnt[e]++;
        }
        void remove(int v) {
            in_S[v] = 0;
            for (int e : P->incident[v]) cnt[e]--;
        }
    };
};

HypergraphCut read_hypergraph(const string& path) {
    ifstream in(path);
    if (!in) { perror(path.c_str()); exit(1); }
    HypergraphCut H;
    int n = 0;
    string line;
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        istringstream ss(line);
        double wt;
        if (!(ss >> wt)) continue;
        vector<int> e;
        int v;
        while (ss >> v) e.push_back(v), n = max(n, v + 1);
        sort(e.begin(), e.end());
        e.erase(unique(e.begin(), e.end()), e.end());
        if (e.size() < 2) continue;
        H.edges.push_back(e);
        H.w.push_back(wt);
    }
    H.finalize(n);
    return H;
}

HypergraphCut synthetic_hypergraph(int n, int m, int smin, int smax, unsigned long long seed) {
    mt19937_64 rng(seed * 6364136223846793005ULL + 1442695040888963407ULL);
    uniform_int_distribution<int> size(smin, smax), wt(1, 9);
    HypergraphCut H;
    vector<int> nodes(n);
    iota(nodes.begin(), nodes.end(), 0);
    for (int e = 0; e < m; e++) {
        int s = min(size(rng), n);
        for (int i = 0; i < s; i++) swap(nodes[i], nodes[i + rng() % (n - i)]);  // partial shuffle
        vector<int> edge(nodes.begin(), nodes.begin() + s);
        sort(edge.begin(), edge.end());
        H.edges.push_back(edge);
        H.w.push_back(wt(rng));
    }
    H.finalize(n);
    return H;
}

int main(int argc, char** argv) {
    Args a(argc, argv);
    unsigned long long seed = a.integer("seed", 0);
    HypergraphCut H;
    string name;
    if (a.has("hypergraph")) {
        H = read_hypergraph(a.str("hypergraph"));
        name = a.str("hypergraph");
        name = name.substr(name.find_last_of('/') + 1);
        name = name.substr(0, name.find('.'));
    } else if (a.has("synthetic")) {
        int n = a.integer("synthetic", 20), m = a.integer("edges", 3 * n);
        H = synthetic_hypergraph(n, m, a.integer("min-size", 2), a.integer("max-size", 5), seed);
        name = "synthetic_n" + to_string(n) + "_m" + to_string(m);
    } else {
        fprintf(stderr, "need --hypergraph FILE or --synthetic N\n");
        return 1;
    }
    printf("hypergraph cut: n=%d, hyperedges=%zu, total weight=%.1f\n", H.n(), H.edges.size(), H.total);
    run_k_sweep(H, sweep_config(a, H.n(), "hypergraph_cut", name, H.total));
}

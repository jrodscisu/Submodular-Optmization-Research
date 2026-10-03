// Regression check for the generic port in dual_core.hpp: re-implements max-cut as a
// problem type and compares OPT, top-k, plain greedy and the dual bound against the CSV
// written by the original src/maxcut_random_greedy.cpp (src/results/k_sweep_n20_p0.3.csv).
//
// Build: g++ -O2 -std=c++17 port_check.cpp -o port_check
// Run:   ./port_check ../src/results/k_sweep_n20_p0.3.csv
#include <cstdio>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "experiment.hpp"

using namespace std;
using Matrix = vector<vector<double>>;

// identical to src/maxcut_random_greedy.cpp
Matrix synthetic_graph(int n, double p, mt19937_64& rng) {
    Matrix W(n, vector<double>(n, 0.0));
    uniform_real_distribution<double> U(0, 1);
    uniform_int_distribution<int> wt(1, 9);
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (U(rng) < p) W[i][j] = W[j][i] = wt(rng);
    return W;
}

struct MaxCut {
    Matrix W;
    int n() const { return W.size(); }
    double eval(const vector<char>& in_S) const {
        double c = 0;
        for (int i = 0; i < n(); i++)
            if (in_S[i])
                for (int j = 0; j < n(); j++)
                    if (!in_S[j]) c += W[i][j];
        return c;
    }
    struct State {
        const MaxCut* P;
        vector<double> deg, w_to_S;
        vector<char> in_S;
        explicit State(const MaxCut& p) : P(&p), deg(p.n()), w_to_S(p.n(), 0.0), in_S(p.n(), 0) {
            for (int i = 0; i < p.n(); i++) deg[i] = accumulate(p.W[i].begin(), p.W[i].end(), 0.0);
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

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "../src/results/k_sweep_n20_p0.3.csv";
    ifstream in(path);
    if (!in) { perror(path); return 1; }
    string line;
    getline(in, line);
    vector<string> header;
    { stringstream ss(line); string h; while (getline(ss, h, ',')) header.push_back(h); }
    auto col = [&](const string& name) { return find(header.begin(), header.end(), name) - header.begin(); };

    int rows = 0, bad = 0;
    map<pair<int, unsigned long long>, MaxCut> cache;
    while (getline(in, line)) {
        vector<string> f;
        stringstream ss(line); string x;
        while (getline(ss, x, ',')) f.push_back(x);
        int n = stoi(f[col("n")]), k = stoi(f[col("k")]);
        double p = stod(f[col("p")]);
        unsigned long long seed = stoull(f[col("seed")]);
        mt19937_64 rng(seed);
        MaxCut P{synthetic_graph(n, p, rng)};

        auto [g, chain] = dual::plain_greedy(P, k);
        double dual_direct = dual::dual_wrapper(P, k, chain);
        double topk = dual::top_k_upper_bound(dual::singleton_gains(P), k);
        auto best = dual::brute_force_by_size(P, k);
        double opt = 0;
        for (double b : best) opt = max(opt, b);

        auto close = [](double a, double b) { return fabs(a - b) <= 5e-4 * max(1.0, fabs(b)); };
        bool ok = close(dual_direct, stod(f[col("dual_bound")])) && close(topk, stod(f[col("top_k_bound")])) &&
                  close(g, stod(f[col("greedy")])) && close(opt, stod(f[col("opt")]));
        if (!ok) {
            bad++;
            printf("MISMATCH seed=%llu k=%d: dual %.4f vs %s, topk %.1f vs %s, greedy %.1f vs %s, opt %.1f vs %s\n",
                   seed, k, dual_direct, f[col("dual_bound")].c_str(), topk, f[col("top_k_bound")].c_str(), g,
                   f[col("greedy")].c_str(), opt, f[col("opt")].c_str());
        }
        rows++;
    }
    printf("%d rows checked, %d mismatches\n", rows, bad);
    return bad != 0;
}

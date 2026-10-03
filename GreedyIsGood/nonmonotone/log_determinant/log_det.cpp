// Log-determinant / DPP MAP under |S| <= k:   f(S) = log det(L_S),  f({}) = 0
// L = alpha * (K + jitter * I) with K an RBF kernel on standardized features,
// K_ij = exp(-||x_i - x_j||^2 / (2 sigma^2)), sigma = bw * (median pairwise distance).
// f is submodular; it is non-monotone once L_S has eigenvalues below 1, which the scale alpha
// tunes (alpha > 1 makes singletons positive, large sets eventually decrease f).
// NOTE: f is not non-negative (log det can drop below 0 for large S), so the 1/e guarantee
// of random greedy does not formally apply; f({}) = 0 keeps OPT >= 0.
//
// With M = L_S^{-1} maintained (common/inverse_set.hpp):
//   gain(v | S) = log(L_vv - L_vS M L_Sv)   for v not in S   (O(|S|^2))
//              = -log(M_vv)                 for v in S        (O(1))
//   top-k bound: k largest positive log L_vv  (Hadamard's inequality);
//   total bound: none (n/a for this objective), written as nan
//
// Build: g++ -O2 -std=c++17 log_det.cpp -o log_det
// Instances:
//   --features FILE [--sample N]   numeric CSV (',' or ';' separated, header lines skipped),
//                                   last column dropped with --drop-last; N rows sampled (seeded)
//   --synthetic N [--dim D]        Gaussian mixture with 5 clusters in R^D (seeded)
//   --alpha A (default 3) --bw B (default 1) --jitter J (default 1e-3)
// Sweep options: see common/experiment.hpp
#include <fstream>

#include "../common/experiment.hpp"
#include "../common/inverse_set.hpp"

using namespace std;
using namespace dual;

struct LogDet {
    Dense L;
    int n() const { return L.n; }

    double eval(const vector<char>& S) const {
        vector<int> A;
        for (int i = 0; i < L.n; i++) if (S[i]) A.push_back(i);
        return logdet_sub(L, A);
    }

    struct State {
        InverseSet inv;
        vector<char> in_S;
        explicit State(const LogDet& p) : inv(&p.L), in_S(p.L.n, 0) {}
        double gain(int v) const { return in_S[v] ? -log(inv.inv_diag(v)) : log(inv.cond_var(v)); }
        void add(int v) { in_S[v] = 1; inv.add(v); }
        void remove(int v) { in_S[v] = 0; inv.remove(v); }
    };
};

vector<vector<double>> read_features(const string& path, bool drop_last) {
    ifstream in(path);
    if (!in) { perror(path.c_str()); exit(1); }
    vector<vector<double>> X;
    string line;
    while (getline(in, line)) {
        for (char& c : line) if (c == ';' || c == ',' || c == '\t') c = ' ';
        istringstream ss(line);
        vector<double> row;
        string tok;
        bool numeric = true;
        while (ss >> tok) {
            char* end;
            double v = strtod(tok.c_str(), &end);
            if (*end) { numeric = false; break; }
            row.push_back(v);
        }
        if (!numeric || row.empty()) continue;
        if (drop_last) row.pop_back();
        X.push_back(row);
    }
    return X;
}

vector<vector<double>> gaussian_mixture(int n, int d, unsigned long long seed) {
    mt19937_64 rng(seed * 31337ULL + 5);
    normal_distribution<double> N01(0, 1);
    vector<vector<double>> centers(5, vector<double>(d));
    for (auto& c : centers) for (auto& x : c) x = 3 * N01(rng);
    vector<vector<double>> X(n, vector<double>(d));
    for (int i = 0; i < n; i++) {
        auto& c = centers[rng() % 5];
        for (int j = 0; j < d; j++) X[i][j] = c[j] + N01(rng);
    }
    return X;
}

Dense rbf_kernel(vector<vector<double>> X, double alpha, double bw, double jitter) {
    int n = X.size(), d = X[0].size();
    for (int j = 0; j < d; j++) {  // standardize columns
        double mu = 0, var = 0;
        for (auto& r : X) mu += r[j];
        mu /= n;
        for (auto& r : X) var += (r[j] - mu) * (r[j] - mu);
        double sd = sqrt(var / n);
        for (auto& r : X) r[j] = sd > 0 ? (r[j] - mu) / sd : 0;
    }
    Dense D2(n);
    vector<double> dists;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++) {
            double s = 0;
            for (int t = 0; t < d; t++) s += (X[i][t] - X[j][t]) * (X[i][t] - X[j][t]);
            D2(i, j) = D2(j, i) = s;
            dists.push_back(sqrt(s));
        }
    nth_element(dists.begin(), dists.begin() + dists.size() / 2, dists.end());
    double sigma = bw * dists[dists.size() / 2];
    Dense L(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            L(i, j) = alpha * (exp(-D2(i, j) / (2 * sigma * sigma)) + (i == j ? jitter : 0.0));
    printf("RBF kernel: n=%d, d=%d, sigma=%.4f, alpha=%.3f, jitter=%.1e\n", n, d, sigma, alpha, jitter);
    return L;
}

int main(int argc, char** argv) {
    Args a(argc, argv);
    unsigned long long seed = a.integer("seed", 0);
    vector<vector<double>> X;
    string name;
    if (a.has("features")) {
        X = read_features(a.str("features"), a.has("drop-last"));
        int sample = a.integer("sample", 0);
        if (sample > 0 && sample < (int)X.size()) {
            mt19937_64 rng(seed * 104729ULL + 11);
            shuffle(X.begin(), X.end(), rng);
            X.resize(sample);
        }
        name = a.str("features");
        name = name.substr(name.find_last_of('/') + 1);
        name = name.substr(0, name.find('.')) + "_n" + to_string(X.size());
    } else if (a.has("synthetic")) {
        X = gaussian_mixture(a.integer("synthetic", 20), a.integer("dim", 5), seed);
        name = "synthetic_n" + to_string(X.size());
    } else {
        fprintf(stderr, "need --features FILE or --synthetic N\n");
        return 1;
    }
    name += "_alpha" + a.str("alpha", "3");
    LogDet P{rbf_kernel(X, a.num("alpha", 3.0), a.num("bw", 1.0), a.num("jitter", 1e-3))};
    run_k_sweep(P, sweep_config(a, P.n(), "log_det", name, NAN));
}

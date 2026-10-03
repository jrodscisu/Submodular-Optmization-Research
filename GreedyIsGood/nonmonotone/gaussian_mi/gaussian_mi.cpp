// Gaussian mutual information under |S| <= k:
//   f(S) = I(X_S ; X_{V\S}) = 1/2 [ log det C_S + log det C_{V\S} - log det C_V ]
// for a Gaussian vector with covariance C. Symmetric, submodular, non-negative, non-monotone
// (f({}) = f(V) = 0). Used for sensor placement (Krause et al.) on the Intel Berkeley lab data.
//
// With inverses of C_S and C_{V\S} maintained (common/inverse_set.hpp), for A = S - v:
//   gain(v | S) = 1/2 log Var(v | A) - 1/2 log Var(v | V \ A \ v)          O(n^2)
//   top-k bound: k largest positive f({v});  total bound: none (n/a), written as nan
//
// Build: g++ -O2 -std=c++17 gaussian_mi.cpp -o gaussian_mi
// Instances:
//   --cov FILE                 "n" then an n x n covariance matrix (whitespace separated)
//   --synthetic N [--ls L]     GP on N uniform points in [0,1]^2, squared-exponential kernel
//                              with length scale L (default 0.2), seeded
//   --nugget E (default 1e-3)  adds E * mean(diag C) to the diagonal (numerical conditioning)
// Sweep options: see common/experiment.hpp
#include <fstream>

#include "../common/experiment.hpp"
#include "../common/inverse_set.hpp"

using namespace std;
using namespace dual;

struct GaussianMI {
    Dense C;
    double logdet_V = 0;
    InverseSet full;  // inverse of C_V, copied into every State's complement set

    explicit GaussianMI(Dense C_) : C(std::move(C_)) {
        vector<int> all(C.n);
        iota(all.begin(), all.end(), 0);
        logdet_V = logdet_sub(C, all);
        full = InverseSet(&C);
        for (int v = 0; v < C.n; v++) full.add(v);
        full.refresh();
    }
    GaussianMI(const GaussianMI&) = delete;  // States hold pointers to C
    int n() const { return C.n; }

    double eval(const vector<char>& S) const {
        vector<int> A, B;
        for (int i = 0; i < C.n; i++) (S[i] ? A : B).push_back(i);
        return 0.5 * (logdet_sub(C, A) + logdet_sub(C, B) - logdet_V);
    }

    struct State {
        InverseSet in, out;  // C_S^{-1}, C_{V\S}^{-1}
        vector<char> in_S;
        explicit State(const GaussianMI& p) : in(&p.C), out(p.full), in_S(p.C.n, 0) {}
        double gain(int v) const {
            if (!in_S[v]) return 0.5 * (log(in.cond_var(v)) + log(out.inv_diag(v)));
            return 0.5 * (-log(in.inv_diag(v)) - log(out.cond_var(v)));
        }
        void add(int v) { in_S[v] = 1; in.add(v); out.remove(v); }
        void remove(int v) { in_S[v] = 0; out.add(v); in.remove(v); }
    };
};

Dense read_cov(const string& path) {
    ifstream in(path);
    if (!in) { perror(path.c_str()); exit(1); }
    int n;
    in >> n;
    Dense C(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) in >> C(i, j);
    if (!in) { fprintf(stderr, "bad covariance file %s\n", path.c_str()); exit(1); }
    return C;
}

Dense gp_cov(int n, double ls, unsigned long long seed) {
    mt19937_64 rng(seed * 2654435761ULL + 7);
    uniform_real_distribution<double> U(0, 1);
    vector<double> x(n), y(n);
    for (int i = 0; i < n; i++) x[i] = U(rng), y[i] = U(rng);
    Dense C(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double d2 = (x[i] - x[j]) * (x[i] - x[j]) + (y[i] - y[j]) * (y[i] - y[j]);
            C(i, j) = exp(-d2 / (2 * ls * ls));
        }
    return C;
}

int main(int argc, char** argv) {
    Args a(argc, argv);
    unsigned long long seed = a.integer("seed", 0);
    Dense C;
    string name;
    if (a.has("cov")) {
        C = read_cov(a.str("cov"));
        name = a.str("cov");
        name = name.substr(name.find_last_of('/') + 1);
        name = name.substr(0, name.find('.'));
    } else if (a.has("synthetic")) {
        C = gp_cov(a.integer("synthetic", 20), a.num("ls", 0.2), seed);
        name = "synthetic_gp_n" + to_string(C.n);
    } else {
        fprintf(stderr, "need --cov FILE or --synthetic N\n");
        return 1;
    }
    double nugget = a.num("nugget", 1e-3), mean_diag = 0;
    for (int i = 0; i < C.n; i++) mean_diag += C(i, i) / C.n;
    for (int i = 0; i < C.n; i++) C(i, i) += nugget * mean_diag;
    GaussianMI P(std::move(C));
    printf("gaussian MI: n=%d, nugget=%.1e, log det C_V=%.3f\n", P.n(), nugget, P.logdet_V);
    run_k_sweep(P, sweep_config(a, P.n(), "gaussian_mi", name, NAN));
}

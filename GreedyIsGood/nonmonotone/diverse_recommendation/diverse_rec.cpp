// Diverse recommendation (Mirzasoleiman et al. 2016, movie recommendation) under |S| <= k:
//   f(S) = sum_{i in S} sum_{j in V} s_ij  -  lambda * sum_{i in S} sum_{j in S} s_ij
// with s_ij >= 0 the cosine similarity of the movies' rating vectors (s_ii = 1).
// Submodular for lambda >= 0, non-negative for lambda <= 1, monotone for lambda <= 1/2;
// non-monotone for lambda in (1/2, 1]  (lambda = 1 is the cut function of the similarity graph).
//
// With t_u = sum_{j in S} s_uj and R_u = sum_j s_uj maintained:
//   gain(v | S) = R_v - lambda * (s_vv + 2 (t_v - s_vv [v in S]))   O(1); add/remove O(n)
//   top-k bound: k largest R_v - lambda s_vv;  total bound: sum_ij s_ij (f(S) <= sum_{i in S} R_i)
//
// Build: g++ -O2 -std=c++17 diverse_rec.cpp -o diverse_rec
// Instances:
//   --movielens ratings.dat --top P [--sample N]   the P most-rated movies, or N of them sampled
//                                                   uniformly at random (seeded)
//   --lambda L (default 1.0)
// Sweep options: see common/experiment.hpp
#include <fstream>
#include <unordered_map>

#include "../common/experiment.hpp"

using namespace std;
using namespace dual;

struct DiverseRec {
    int N = 0;
    double lambda = 1.0;
    vector<double> s;  // N x N similarity
    vector<double> R;  // row sums
    double total = 0;

    double sim(int i, int j) const { return s[(size_t)i * N + j]; }
    int n() const { return N; }

    void finalize() {
        R.assign(N, 0.0);
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++) R[i] += sim(i, j);
        total = accumulate(R.begin(), R.end(), 0.0);
    }

    double eval(const vector<char>& S) const {
        double f = 0;
        for (int i = 0; i < N; i++) {
            if (!S[i]) continue;
            f += R[i];
            for (int j = 0; j < N; j++)
                if (S[j]) f -= lambda * sim(i, j);
        }
        return f;
    }

    struct State {
        const DiverseRec* P;
        vector<char> in_S;
        vector<double> t;  // t_u = sum_{j in S} s_uj
        explicit State(const DiverseRec& p) : P(&p), in_S(p.N, 0), t(p.N, 0.0) {}
        double gain(int v) const {
            double svv = P->sim(v, v);
            return P->R[v] - P->lambda * (svv + 2 * (t[v] - (in_S[v] ? svv : 0.0)));
        }
        void add(int v) {
            in_S[v] = 1;
            const double* row = &P->s[(size_t)v * P->N];
            for (int u = 0; u < P->N; u++) t[u] += row[u];
        }
        void remove(int v) {
            in_S[v] = 0;
            const double* row = &P->s[(size_t)v * P->N];
            for (int u = 0; u < P->N; u++) t[u] -= row[u];
        }
    };
};

// cosine similarity of rating vectors for a selection of movies from MovieLens ratings.dat
// ("UserID::MovieID::Rating::Timestamp")
DiverseRec movielens(const string& path, int top, int sample, unsigned long long seed) {
    ifstream in(path);
    if (!in) { perror(path.c_str()); exit(1); }
    struct R { int u, m; double r; };
    vector<R> ratings;
    unordered_map<int, int> count;
    string line;
    while (getline(in, line)) {
        int u, m;
        double r;
        if (sscanf(line.c_str(), "%d::%d::%lf", &u, &m, &r) != 3) continue;
        ratings.push_back({u, m, r});
        count[m]++;
    }
    vector<pair<int, int>> by_count;  // (-count, movie) for a deterministic order
    for (auto [m, c] : count) by_count.push_back({-c, m});
    sort(by_count.begin(), by_count.end());
    top = min<int>(top, by_count.size());
    vector<int> movies;
    for (int i = 0; i < top; i++) movies.push_back(by_count[i].second);
    if (sample > 0 && sample < top) {
        mt19937_64 rng(seed * 104729ULL + 3);
        shuffle(movies.begin(), movies.end(), rng);
        movies.resize(sample);
        sort(movies.begin(), movies.end());
    }
    unordered_map<int, int> col;
    for (size_t i = 0; i < movies.size(); i++) col[movies[i]] = i;
    unordered_map<int, int> user;
    for (auto& r : ratings) if (col.count(r.m) && !user.count(r.u)) { int k = user.size(); user[r.u] = k; }

    int N = movies.size(), U = user.size();
    vector<double> X((size_t)N * U, 0.0);
    for (auto& r : ratings) {
        auto it = col.find(r.m);
        if (it != col.end()) X[(size_t)it->second * U + user[r.u]] = r.r;
    }
    vector<double> norm(N, 0.0);
    for (int i = 0; i < N; i++)
        for (int u = 0; u < U; u++) norm[i] += X[(size_t)i * U + u] * X[(size_t)i * U + u];
    for (auto& x : norm) x = sqrt(x);

    DiverseRec P;
    P.N = N;
    P.s.assign((size_t)N * N, 0.0);
    for (int i = 0; i < N; i++)
        for (int j = i; j < N; j++) {
            double d = 0;
            for (int u = 0; u < U; u++) d += X[(size_t)i * U + u] * X[(size_t)j * U + u];
            double c = (i == j) ? 1.0 : d / (norm[i] * norm[j]);
            P.s[(size_t)i * N + j] = P.s[(size_t)j * N + i] = c;
        }
    printf("movielens: %d movies (top %d by #ratings%s), %d users\n", N, top,
           sample > 0 ? ", random sample" : "", U);
    return P;
}

int main(int argc, char** argv) {
    Args a(argc, argv);
    unsigned long long seed = a.integer("seed", 0);
    if (!a.has("movielens")) {
        fprintf(stderr, "need --movielens ratings.dat --top P [--sample N]\n");
        return 1;
    }
    int top = a.integer("top", 500), sample = a.integer("sample", 0);
    DiverseRec P = movielens(a.str("movielens"), top, sample, seed);
    P.lambda = a.num("lambda", 1.0);
    P.finalize();
    string name = "movielens_" + (sample > 0 ? "sample" + to_string(sample) + "of" : string("top")) +
                  to_string(top) + "_lambda" + a.str("lambda", "1.0");
    printf("diverse recommendation: n=%d, lambda=%.3f, total bound=%.3f\n", P.n(), P.lambda, P.total);
    run_k_sweep(P, sweep_config(a, P.n(), "diverse_rec", name, P.total));
}

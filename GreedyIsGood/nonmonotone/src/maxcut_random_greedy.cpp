// Max-cut under a cardinality constraint |S| <= k via Random Greedy
// (Buchbinder, Feldman, Naor, Schwartz 2014): 1/e-approximation in expectation
// for non-monotone submodular maximization.
// f(S) = weight of edges crossing (S, V \ S).
//
// Build: g++ -O2 -std=c++17 maxcut_random_greedy.cpp -o maxcut
// Run:   ./maxcut [n p k trials seed csv_path]
//        Results are appended as one row to csv_path (default: maxcut_results.csv).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <random>
#include <vector>
#include <cassert>
#include <cfloat>
#include <iostream> 

using namespace std;

using Matrix = vector<vector<double>>;

Matrix synthetic_graph(int n, double p, mt19937_64& rng) {
    Matrix W(n, vector<double>(n, 0.0));
    uniform_real_distribution<double> U(0, 1);
    uniform_int_distribution<int> wt(1, 9);
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (U(rng) < p) W[i][j] = W[j][i] = wt(rng);
    return W;
}

double cut_value(const Matrix& W, const vector<char>& in_S) {
    int n = W.size();
    double c = 0;
    for (int i = 0; i < n; i++)
        if (in_S[i])
            for (int j = 0; j < n; j++)
                if (!in_S[j]) c += W[i][j];
    return c;
}

// gain(v | S) = w(v, V\S) - w(v, S) = deg(v) - 2 w(v, S)
struct State {
    const Matrix& W;
    vector<double> deg, w_to_S;
    vector<char> in_S;
    
    explicit State(const Matrix& W_) : W(W_), deg(W_.size()), w_to_S(W_.size(), 0.0), in_S(W_.size(), 0) {
        for (size_t i = 0; i < W.size(); i++) deg[i] = accumulate(W[i].begin(), W[i].end(), 0.0);
    }
    double gain(int v) const { return deg[v] - 2 * w_to_S[v]; }
    void add(int v) {
        in_S[v] = 1;
        for (size_t u = 0; u < W.size(); u++) w_to_S[u] += W[v][u];
    }
    void remove(int v) {
        in_S[v] = 0;
        for (size_t u = 0; u < W.size(); u++) w_to_S[u] -= W[v][u];
    }
};

double random_greedy(const Matrix& W, int k, mt19937_64& rng) {
    int n = W.size();
    State st(W);
    for (int it = 0; it < k; it++) {
        // top-k candidates by marginal gain among elements not in S
        vector<int> cand;
        for (int v = 0; v < n; v++) if (!st.in_S[v]) cand.push_back(v);
        int m = min<int>(k, cand.size());
        partial_sort(cand.begin(), cand.begin() + m, cand.end(),
                     [&](int a, int b) { return st.gain(a) > st.gain(b); });
        cand.resize(m);
        // non-positive gains <-> zero-gain dummies
        while (!cand.empty() && st.gain(cand.back()) <= 0) cand.pop_back();
        if(cand.empty()) break;
        int j = uniform_int_distribution<int>(0, k - 1)(rng);  // uniform over k slots
        if (j < (int)cand.size()) st.add(cand[j]);
    }
    return cut_value(W, st.in_S);
}

pair<double, vector<vector<char>>> plain_greedy(const Matrix& W, int k) {
    int n = W.size();
    vector<vector<char>> S_collection;
    State st(W);
    for (int it = 0; it < k; it++) {
        S_collection.push_back(st.in_S);

        int best = -1;
        for (int v = 0; v < n; v++)
            if (!st.in_S[v] && (best < 0 || st.gain(v) > st.gain(best))) best = v;
        if (best < 0 || st.gain(best) <= 0) break;
        st.add(best);
    }

    S_collection.push_back(st.in_S);
    return {cut_value(W, st.in_S), S_collection};
}

// enumerate all subsets of size <= k (small n only)
void brute_rec(const Matrix& W, int start, int left, vector<char>& in_S, double& best) {
    best = max(best, cut_value(W, in_S));
    if (left == 0) return;
    for (int v = start; v < (int)W.size(); v++) {
        in_S[v] = 1;
        brute_rec(W, v + 1, left - 1, in_S, best);
        in_S[v] = 0;
    }
}

double brute_force(const Matrix& W, int k) {
    vector<char> in_S(W.size(), 0);
    double best = 0;
    brute_rec(W, 0, k, in_S, best);
    return best;
}

double top_k_upper_bound(const Matrix& W, int k){
    int n = W.size();
    vector<double> deg(n);
    for (int i = 0; i < n; i++) deg[i] = accumulate(W[i].begin(), W[i].end(), 0.0);
    sort(deg.begin(), deg.end(), greater<double>());
    double ub = 0;
    for (int i = 0; i < min(k, n); i++) ub += deg[i];
    return ub;
}

double penalty_S(const Matrix& W, const vector<char>& S) {
    State st(W);
    int n = W.size();
    for(int i = 0; i < n; i++) {
        st.add(i);
    }
    
    double non_negative_sum = 0.0;

    for(int j = 0; j < n; j++) {
        if(S[j] == 0) continue;
        st.remove(j);
        non_negative_sum += max(0.0, -st.gain(j));
        st.add(j);
    }
    return non_negative_sum;
}

vector<double> high_cap_U(State & st, vector<int>& order) {
    int m = order.size();
    vector<double> U(m, 0.0), U_tilde(m);
    State x_st = st;

    double penalty;
    double Ai_minus_1 = 0.0, X_minus_1 = 0.0;
    
    for(int i = 0; i < m; i++) {
        double Ai = st.gain(order[i]) + Ai_minus_1;
        st.add(order[i]);
        penalty = max(0.0, Ai_minus_1 - Ai);

        for(int j = 0; j < i; j++) {
            st.remove(order[j]);
            double gain = st.gain(order[j]);
            penalty += max(0.0, -gain);
            st.add(order[j]);
        }
        U[i] = Ai + penalty;
        Ai_minus_1 = Ai;

        penalty = 0.0;

        double Xi;
        if(x_st.gain(order[i]) > 0) {
            Xi = x_st.gain(order[i]) + X_minus_1;
            x_st.add(order[i]);
        }else{
            Xi = X_minus_1;
        }       

        for(int j = 0; j <= i; j++) {
            if(x_st.in_S[order[j]]){
                st.remove(order[j]);
                penalty += max(0.0, -st.gain(order[j]));
                st.add(order[j]);
            }else{
                x_st.add(order[j]);
                penalty += max(0.0, x_st.gain(order[j]));
                x_st.remove(order[j]);
            }
        }

        X_minus_1 = Xi;

        U[i] = min(U[i], Xi + penalty);
    }

    U_tilde[m-1] = U[m-1];

    for(int i = m - 2; i >= 0; i--) {
        U_tilde[i] = min(U[i], U_tilde[i+1]);
    }

    return U_tilde;
}

double dual_upper_bound(const Matrix& W, int k, vector<char>& S) {
    int n = W.size();
    State st(W);

    for(int v = 0; v < n; v++){
        if(S[v] == 1){
            st.add(v);
        }
    }

    vector<double> single(n, 0.0);
    vector<int> order;
    for (int i = 0; i < n; i++) {
        if(!S[i]){
            order.push_back(i);
            single[i] = st.gain(i);
        }
    }

    sort(order.begin(), order.end(), [&single](int a, int b) { return single[a] > single[b]; });

    int m = order.size();
    if(m == 0) return 0.0;

    vector<double> U_tilde = high_cap_U(st, order);
    vector<vector<double>> dp(k + 1, vector<double>(m + 1, 0.0));

    for(int j = 1; j <= k; j++) {
        dp[j][0] = -DBL_MAX;
    }

    for(int j = 1; j <= k; j++) {
        for(int i = 1; i <= m; i++) {
            // order[i-1] because order is 0-indexed and dp is 1-indexed
            // U_tilde[i-1] because U_tilde is 0-indexed and dp is 1-indexed
            dp[j][i] = max(dp[j][i-1], min(dp[j-1][i-1] + single[order[i-1]], U_tilde[i-1]));
        }
    }

    double dual_bound = 0.0;

    for(int j = 0; j <= k; j++) {
        dual_bound = max(dual_bound, dp[j][m]);
    }
    return dual_bound;
}

double dual_wrapper(const Matrix& W, int k, vector<vector<char>>& S_collection) {
    double opt = DBL_MAX;

    vector<vector<char>>pruned_S_collection;

    pruned_S_collection = S_collection;


    for (const auto& S : pruned_S_collection) {
        double dual_bound = dual_upper_bound(W, k, const_cast<vector<char>&>(S));
        // printf("f(S) = %.1f, penalty(S) = %.1f, dual_bound = %.1f\n", cut_value(W, S), penalty_S(W, S), dual_bound);
        opt = min(opt, cut_value(W, S) + penalty_S(W, S) + dual_bound);
        printf("Current best dual bound: %.1f\n", opt);
    }
    return opt;
}

int main(int argc, char** argv) {
    int n = argc > 1 ? atoi(argv[1]) : 20;
    double p = argc > 2 ? atof(argv[2]) : 0.3;
    int k = argc > 3 ? atoi(argv[3]) : 6;
    int trials = argc > 4 ? atoi(argv[4]) : 2000;
    unsigned long long seed = argc > 5 ? strtoull(argv[5], nullptr, 10) : 0;
    const char* csv_path = argc > 6 ? argv[6] : "maxcut_results.csv";

    mt19937_64 rng(seed);
    Matrix W = synthetic_graph(n, p, rng);
    int edges = 0;
    double total_edge_weights = 0.0;
    for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) edges += W[i][j] > 0, total_edge_weights += W[i][j];
    printf("G(n=%d, p=%.2f), edges=%d, k=%d\n", n, p, edges, k);

    double opt = (n <= 20) ? brute_force(W, k) : 0.0;
    auto [g, S_collection] = plain_greedy(W, k);
    double ub = top_k_upper_bound(W, k);
    double dual_bound = dual_wrapper(W, k, S_collection);

    double sum = 0, mn = 1e18, mx = -1e18;
    for (int t = 0; t < trials; t++) {
        double v = random_greedy(W, k, rng);
        sum += v; mn = min(mn, v); mx = max(mx, v);
    }
    double mean = sum / trials;
    printf("OPT (brute force)      : %.1f\n", opt);
    printf("Total sum of weights.  : %.1f  ratio=%.3f\n", total_edge_weights, total_edge_weights / opt);
    printf("Top-k upper bound      : %.1f  ratio=%.3f\n", ub, ub / opt);    
    printf("Dual upper bound       : %.1f  ratio=%.3f\n", dual_bound, dual_bound / opt);
    
    assert(dual_bound >= opt - 1e-6 * max(1.0, fabs(opt)));

    printf("Plain greedy           : %.1f  ratio=%.3f  dual_ratio=%.3f\n", g, g / opt, g / dual_bound);
    printf("Random greedy (%d runs): mean=%.1f  ratio=%.3f  dual_ratio=%.3f top_k_ratio=%.3f total_weight_ratio=%.3f  min=%.1f  max=%.1f\n",
           trials, mean, mean / opt, mean / dual_bound, mean / ub, mean /total_edge_weights, mn, mx);
    printf("1/e guarantee          : %.1f  dual=%.3f (%s)\n", opt / M_E, dual_bound/ M_E, mean >= opt / M_E ? "OK" : "VIOLATED");

    // append results to CSV, writing the header if the file is new/empty
    FILE* csv = fopen(csv_path, "a+");
    if (!csv) { perror(csv_path); return 1; }
    fseek(csv, 0, SEEK_END);
    if (ftell(csv) == 0)
        fprintf(csv, "n,p,k,trials,seed,edges,opt,dual_bound,dual_ratio_opt,greedy,greedy_ratio,greedy_dual_ratio,"
                     "rg_top_k_ratio,rg_total_weight_ratio,rg_mean,rg_ratio,rg_dual_ratio,rg_min,rg_max,one_over_e_bound,one_over_e_ok\n");
    fprintf(csv, "%d,%.2f,%d,%d,%llu,%d,%.1f,%.1f,%.6f,%.1f,%.6f,%.6f,%.6f,%.6f,%.3f,%.6f,%.6f,%.1f,%.1f,%.3f,%d\n",
            n, p, k, trials, seed, edges, opt, dual_bound, dual_bound / opt, mean, mean / opt, mean / dual_bound,
            mean / ub, mean / total_edge_weights, mean, mean / opt, mean / dual_bound, mn, mx, opt / M_E, mean >= opt / M_E ? 1 : 0);
    fclose(csv);

}

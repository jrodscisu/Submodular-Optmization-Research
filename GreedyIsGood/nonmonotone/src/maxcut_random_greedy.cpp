// Max-cut under a cardinality constraint |S| <= k via Random Greedy
// (Buchbinder, Feldman, Naor, Schwartz 2014): 1/e-approximation in expectation
// for non-monotone submodular maximization.
// f(S) = weight of edges crossing (S, V \ S).
//
// Build: g++ -O2 -std=c++17 maxcut_random_greedy.cpp -o maxcut
// Run:   ./maxcut [n p k trials seed csv_path brute_max_n prefix_csv_path]
//        Results are appended as one row to csv_path (default: maxcut_results.csv).
//        OPT is brute-forced only when n <= brute_max_n (default 20); otherwise opt and
//        every opt-ratio are written as nan.
//        If prefix_csv_path is given, one row per greedy prefix S_i is appended there with
//        the terms f(S_i) + penalty(S_i) + dual_upper_bound(S_i) that dual_wrapper minimizes.
#include <algorithm>
#include <chrono>
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

static double ms_since(chrono::steady_clock::time_point t0) {
    return chrono::duration<double, milli>(chrono::steady_clock::now() - t0).count();
}

int main(int argc, char** argv) {
    int n = argc > 1 ? atoi(argv[1]) : 20;
    double p = argc > 2 ? atof(argv[2]) : 0.3;
    int k = argc > 3 ? atoi(argv[3]) : 6;
    int trials = argc > 4 ? atoi(argv[4]) : 2000;
    unsigned long long seed = argc > 5 ? strtoull(argv[5], nullptr, 10) : 0;
    const char* csv_path = argc > 6 ? argv[6] : "maxcut_results.csv";
    int brute_max_n = argc > 7 ? atoi(argv[7]) : 20;
    const char* prefix_csv_path = argc > 8 ? argv[8] : nullptr;

    mt19937_64 rng(seed);
    Matrix W = synthetic_graph(n, p, rng);
    int edges = 0;
    double total_edge_weights = 0.0;
    for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) edges += W[i][j] > 0, total_edge_weights += W[i][j];
    printf("G(n=%d, p=%.2f), edges=%d, k=%d\n", n, p, edges, k);

    auto t0 = chrono::steady_clock::now();
    bool has_opt = n <= brute_max_n;
    double opt = has_opt ? brute_force(W, k) : NAN;
    double time_opt_ms = has_opt ? ms_since(t0) : NAN;

    t0 = chrono::steady_clock::now();
    auto [g, S_collection] = plain_greedy(W, k);
    double time_greedy_ms = ms_since(t0);

    double ub = top_k_upper_bound(W, k);

    t0 = chrono::steady_clock::now();
    double dual_bound = dual_wrapper(W, k, S_collection);
    double time_dual_ms = ms_since(t0);

    // dual bound from S_0 = {} alone, i.e. without minimizing over the greedy chain
    double dual_bound_S0 = dual_upper_bound(W, k, S_collection[0]);

    t0 = chrono::steady_clock::now();
    double sum = 0, sum_sq = 0, mn = 1e18, mx = -1e18;
    for (int t = 0; t < trials; t++) {
        double v = random_greedy(W, k, rng);
        sum += v; sum_sq += v * v; mn = min(mn, v); mx = max(mx, v);
    }
    double time_rg_ms = ms_since(t0) / trials;
    double mean = sum / trials;
    double sd = sqrt(max(0.0, sum_sq / trials - mean * mean));

    printf("OPT (brute force)      : %.1f\n", opt);
    printf("Total sum of weights.  : %.1f  ratio=%.3f\n", total_edge_weights, total_edge_weights / opt);
    printf("Top-k upper bound      : %.1f  ratio=%.3f\n", ub, ub / opt);
    printf("Dual upper bound       : %.1f  ratio=%.3f\n", dual_bound, dual_bound / opt);

    if (has_opt) assert(dual_bound >= opt - 1e-6 * max(1.0, fabs(opt)));

    printf("Plain greedy           : %.1f  ratio=%.3f  dual_ratio=%.3f\n", g, g / opt, g / dual_bound);
    printf("Random greedy (%d runs): mean=%.1f  ratio=%.3f  dual_ratio=%.3f top_k_ratio=%.3f total_weight_ratio=%.3f  min=%.1f  max=%.1f\n",
           trials, mean, mean / opt, mean / dual_bound, mean / ub, mean /total_edge_weights, mn, mx);
    printf("1/e guarantee          : %.1f  dual=%.3f (%s)\n", opt / M_E, dual_bound/ M_E,
           !has_opt ? "OPT unknown" : mean >= opt / M_E ? "OK" : "VIOLATED");

    // append results to CSV, writing the header if the file is new/empty.
    // x_over_B = value(x) / B. For an upper bound B, rg_over_B is a certified
    // (a-posteriori) approximation ratio that can be compared against 1/e.
    FILE* csv = fopen(csv_path, "a+");
    if (!csv) { perror(csv_path); return 1; }
    fseek(csv, 0, SEEK_END);
    if (ftell(csv) == 0)
        fprintf(csv, "n,p,k,trials,seed,edges,total_weight,opt,top_k_bound,dual_bound,dual_bound_S0,"
                     "greedy,rg_mean,rg_std,rg_min,rg_max,"
                     "dual_over_opt,dual_S0_over_opt,top_k_over_opt,total_over_opt,"
                     "rg_over_opt,rg_over_dual,rg_over_top_k,rg_over_total,"
                     "greedy_over_opt,greedy_over_dual,greedy_over_top_k,greedy_over_total,"
                     "rg_ge_opt_over_e,rg_ge_dual_over_e,"
                     "time_opt_ms,time_greedy_ms,time_dual_ms,time_rg_ms\n");
    fprintf(csv, "%d,%.2f,%d,%d,%llu,%d,%.1f,%.1f,%.1f,%.4f,%.4f,"
                 "%.1f,%.4f,%.4f,%.1f,%.1f,"
                 "%.6f,%.6f,%.6f,%.6f,"
                 "%.6f,%.6f,%.6f,%.6f,"
                 "%.6f,%.6f,%.6f,%.6f,"
                 "%d,%d,"
                 "%.3f,%.3f,%.3f,%.4f\n",
            n, p, k, trials, seed, edges, total_edge_weights, opt, ub, dual_bound, dual_bound_S0,
            g, mean, sd, mn, mx,
            dual_bound / opt, dual_bound_S0 / opt, ub / opt, total_edge_weights / opt,
            mean / opt, mean / dual_bound, mean / ub, mean / total_edge_weights,
            g / opt, g / dual_bound, g / ub, g / total_edge_weights,
            has_opt ? (mean >= opt / M_E ? 1 : 0) : -1, mean >= dual_bound / M_E ? 1 : 0,
            time_opt_ms, time_greedy_ms, time_dual_ms, time_rg_ms);
    fclose(csv);

    // per-prefix breakdown of the quantity minimized in dual_wrapper
    if (prefix_csv_path) {
        FILE* pcsv = fopen(prefix_csv_path, "a+");
        if (!pcsv) { perror(prefix_csv_path); return 1; }
        fseek(pcsv, 0, SEEK_END);
        if (ftell(pcsv) == 0)
            fprintf(pcsv, "n,p,k,seed,opt,prefix,size,f_S,penalty_S,dual_S,bound_S\n");
        for (size_t i = 0; i < S_collection.size(); i++) {
            auto& S = S_collection[i];
            int size = count(S.begin(), S.end(), 1);
            double f = cut_value(W, S), pen = penalty_S(W, S), d = dual_upper_bound(W, k, S);
            fprintf(pcsv, "%d,%.2f,%d,%llu,%.1f,%zu,%d,%.1f,%.4f,%.4f,%.4f\n",
                    n, p, k, seed, opt, i, size, f, pen, d, f + pen + d);
        }
        fclose(pcsv);
    }
}

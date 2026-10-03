// Problem-independent port of the algorithms in src/maxcut_random_greedy.cpp:
// random greedy, plain greedy, brute force, top-k singleton bound, and the dual upper
// bound (penalty_S, high_cap_U, dual_upper_bound, dual_wrapper).
//
// The bodies are line-for-line the max-cut versions with `const Matrix& W` replaced by a
// problem object. A problem type P must provide
//
//   int    n() const;                          ground-set size
//   double eval(const vector<char>& in_S) const;   f(S) computed from scratch
//   struct State {                             incremental oracle for a current set S
//       explicit State(const P&);              S = {}
//       vector<char> in_S;
//       double gain(int v) const;              f(S + v) - f(S - v)   (for v in S or not)
//       void add(int v);  void remove(int v);
//   };                                         (copyable: high_cap_U copies it)
//
// gain(v) must have the max-cut semantics f(S ∪ {v}) − f(S \ {v}) also when v ∈ S:
// high_cap_U calls x_st.gain(v) right after x_st.add(v), exactly as in the max-cut code.
// common/port_check.cpp verifies this file reproduces the max-cut binary bit for bit.
#pragma once
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace dual {

using std::vector;

template <class P>
double random_greedy(const P& prob, int k, std::mt19937_64& rng) {
    int n = prob.n();
    typename P::State st(prob);
    vector<double> g(n);
    for (int it = 0; it < k; it++) {
        // top-k candidates by marginal gain among elements not in S
        vector<int> cand;
        for (int v = 0; v < n; v++) if (!st.in_S[v]) { cand.push_back(v); g[v] = st.gain(v); }
        int m = std::min<int>(k, cand.size());
        std::partial_sort(cand.begin(), cand.begin() + m, cand.end(),
                          [&](int a, int b) { return g[a] > g[b]; });
        cand.resize(m);
        // non-positive gains <-> zero-gain dummies
        while (!cand.empty() && g[cand.back()] <= 0) cand.pop_back();
        if (cand.empty()) break;
        int j = std::uniform_int_distribution<int>(0, k - 1)(rng);  // uniform over k slots
        if (j < (int)cand.size()) st.add(cand[j]);
    }
    return prob.eval(st.in_S);
}

// returns f(S_final) and the chain S_0 = {}, S_1, ..., S_final (last set repeated if greedy stops early)
template <class P>
std::pair<double, vector<vector<char>>> plain_greedy(const P& prob, int k) {
    int n = prob.n();
    vector<vector<char>> S_collection;
    typename P::State st(prob);
    for (int it = 0; it < k; it++) {
        S_collection.push_back(st.in_S);

        int best = -1;
        double best_gain = 0;
        for (int v = 0; v < n; v++)
            if (!st.in_S[v]) {
                double gv = st.gain(v);
                if (best < 0 || gv > best_gain) best = v, best_gain = gv;
            }
        if (best < 0 || best_gain <= 0) break;
        st.add(best);
    }

    S_collection.push_back(st.in_S);
    return {prob.eval(st.in_S), S_collection};
}

// best[s] = max f(S) over |S| = s, s = 0..kb (exhaustive DFS; small instances only)
template <class P>
vector<double> brute_force_by_size(const P& prob, int kb) {
    typename P::State st(prob);
    vector<double> best(kb + 1, -DBL_MAX);
    auto rec = [&](auto&& self, int start, int depth, double cur) -> void {
        best[depth] = std::max(best[depth], cur);
        if (depth == kb) return;
        for (int v = start; v < prob.n(); v++) {
            double g = st.gain(v);
            st.add(v);
            self(self, v + 1, depth + 1, cur + g);
            st.remove(v);
        }
    };
    rec(rec, 0, 0, prob.eval(st.in_S));
    return best;
}

// singleton values f({v}) - f({})
template <class P>
vector<double> singleton_gains(const P& prob) {
    typename P::State st(prob);
    vector<double> s(prob.n());
    for (int v = 0; v < prob.n(); v++) s[v] = st.gain(v);
    return s;
}

// sum of the k largest positive singleton values (max-cut: the k largest degrees)
inline double top_k_upper_bound(vector<double> single, int k) {
    std::sort(single.begin(), single.end(), std::greater<double>());
    double ub = 0;
    for (int i = 0; i < std::min<int>(k, single.size()); i++) ub += std::max(0.0, single[i]);
    return ub;
}

template <class P>
double penalty_S(const P& prob, const vector<char>& S) {
    typename P::State st(prob);
    int n = prob.n();
    for (int i = 0; i < n; i++) {
        st.add(i);
    }

    double non_negative_sum = 0.0;

    for (int j = 0; j < n; j++) {
        if (S[j] == 0) continue;
        st.remove(j);
        non_negative_sum += std::max(0.0, -st.gain(j));
        st.add(j);
    }
    return non_negative_sum;
}

template <class State>
vector<double> high_cap_U(State& st, vector<int>& order) {
    int m = order.size();
    vector<double> U(m, 0.0), U_tilde(m);
    State x_st = st;

    double penalty;
    double Ai_minus_1 = 0.0, X_minus_1 = 0.0;

    for (int i = 0; i < m; i++) {
        double Ai = st.gain(order[i]) + Ai_minus_1;
        st.add(order[i]);
        penalty = std::max(0.0, Ai_minus_1 - Ai);

        for (int j = 0; j < i; j++) {
            st.remove(order[j]);
            double gain = st.gain(order[j]);
            penalty += std::max(0.0, -gain);
            st.add(order[j]);
        }
        U[i] = Ai + penalty;
        Ai_minus_1 = Ai;

        penalty = 0.0;

        double Xi;
        if (x_st.gain(order[i]) > 0) {
            Xi = x_st.gain(order[i]) + X_minus_1;
            x_st.add(order[i]);
        } else {
            Xi = X_minus_1;
        }

        for (int j = 0; j <= i; j++) {
            if (x_st.in_S[order[j]]) {
                st.remove(order[j]);
                penalty += std::max(0.0, -st.gain(order[j]));
                st.add(order[j]);
            } else {
                x_st.add(order[j]);
                penalty += std::max(0.0, x_st.gain(order[j]));
                x_st.remove(order[j]);
            }
        }

        X_minus_1 = Xi;

        U[i] = std::min(U[i], Xi + penalty);
    }

    U_tilde[m - 1] = U[m - 1];

    for (int i = m - 2; i >= 0; i--) {
        U_tilde[i] = std::min(U[i], U_tilde[i + 1]);
    }

    return U_tilde;
}

// dp_rows[j] = dp[j][m] of dual_upper_bound for j = 0..kmax, so that
// dual_upper_bound(k, S) = max(0, max_{j <= k} dp_rows[j]) for every k <= kmax.
// Rows of the DP do not depend on k, which lets a k sweep evaluate each set S once.
template <class P>
vector<double> dual_dp_rows(const P& prob, int kmax, const vector<char>& S) {
    int n = prob.n();
    typename P::State st(prob);

    for (int v = 0; v < n; v++) {
        if (S[v] == 1) {
            st.add(v);
        }
    }

    vector<double> single(n, 0.0);
    vector<int> order;
    for (int i = 0; i < n; i++) {
        if (!S[i]) {
            order.push_back(i);
            single[i] = st.gain(i);
        }
    }

    std::sort(order.begin(), order.end(), [&single](int a, int b) { return single[a] > single[b]; });

    int m = order.size();
    vector<double> rows(kmax + 1, 0.0);
    if (m == 0) return rows;  // dual_upper_bound returns 0.0

    vector<double> U_tilde = high_cap_U(st, order);
    // dp[j][i], kept two rows at a time; rows[j] = dp[j][m]
    vector<double> prev(m + 1, 0.0), cur(m + 1);
    rows[0] = prev[m];
    for (int j = 1; j <= kmax; j++) {
        cur[0] = -DBL_MAX;
        for (int i = 1; i <= m; i++) {
            // order[i-1] because order is 0-indexed and dp is 1-indexed
            // U_tilde[i-1] because U_tilde is 0-indexed and dp is 1-indexed
            cur[i] = std::max(cur[i - 1], std::min(prev[i - 1] + single[order[i - 1]], U_tilde[i - 1]));
        }
        rows[j] = cur[m];
        std::swap(prev, cur);
    }
    return rows;
}

inline double dual_from_rows(const vector<double>& rows, int k) {
    double dual_bound = 0.0;
    for (int j = 0; j <= k && j < (int)rows.size(); j++) {
        dual_bound = std::max(dual_bound, rows[j]);
    }
    return dual_bound;
}

template <class P>
double dual_upper_bound(const P& prob, int k, const vector<char>& S) {
    return dual_from_rows(dual_dp_rows(prob, k, S), k);
}

// min over the greedy chain of f(S) + penalty(S) + dual_upper_bound(S)
template <class P>
double dual_wrapper(const P& prob, int k, const vector<vector<char>>& S_collection) {
    double opt = DBL_MAX;
    for (const auto& S : S_collection) {
        opt = std::min(opt, prob.eval(S) + penalty_S(prob, S) + dual_upper_bound(prob, k, S));
    }
    return opt;
}

}  // namespace dual

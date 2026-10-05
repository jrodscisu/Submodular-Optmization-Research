// Upper-bound baselines B2-B4 (see baselines_report.md), computed with each problem's existing
// oracle (State::gain/add/remove and eval). Notation: f(a | X) = f(X + a) - f(X), [x]^+ = max(0, x).
// State::gain(v) = f(S + v) - f(S - v), so with the state at X:
//   v not in X:  gain(v) = f(v | X);      v in X:  gain(v) = f(v | X - v).
//
//   B2  LP over the base sets S of NM-Dual (exported here, solved in common/baselines_lp.py):
//         max z  s.t.  z <= f(S) + sum_{a not in S} f(a|S) x_a - sum_{a in S} f(a|V-a)(1 - x_a),
//                      sum_a x_a <= k,  x in [0,1]^V
//   B3  Iterative prune -> lattice [A*, B*]; deterministic double greedy on it -> S_D;
//         gamma1 = 3 f(S_D) - f(A*) - f(B*);  gamma1_noprune: the same on [{}, V]
//   B4  mu2(X) = f(X) + sum_{e in X\A*} [-f(e | B*-e)]^+ + sum_{e in B*\X} [f(e | X)]^+
//       mu3(X) = f(X) + sum_{e in X\A*} [-f(e | X-e)]^+  + sum_{e in B*\X} [f(e | A*)]^+
//         minimized over A*, B*, S_D, the base sets projected to the lattice, 5 random lattice sets.
// B3/B4 bound the unconstrained optimum (valid for OPT_k). Oracle values are checked for
// NaN/inf and reported, never replaced.
#pragma once
#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "dual_core.hpp"

namespace dual {

using std::vector;

inline double check_finite(double v, const char* what, int e) {
    if (!std::isfinite(v)) {
        char buf[200];
        snprintf(buf, sizeof buf, "non-finite oracle value %g in %s (element %d)", v, what, e);
        throw std::runtime_error(buf);
    }
    return v;
}

template <class P>
typename P::State state_of(const P& prob, const vector<char>& X) {
    typename P::State st(prob);
    for (int v = 0; v < prob.n(); v++) if (X[v]) st.add(v);
    return st;
}

struct Lattice {
    vector<char> A, B;
    int rounds = 0;
};

// Iterative Prune: A_{t+1} = A_t + {e in B_t\A_t : f(e | B_t - e) >= 0},
//                  B_{t+1} = B_t - {e in B_t\A_t : f(e | A_t) < 0}, until no change.
template <class P>
Lattice iterative_prune(const P& prob) {
    int n = prob.n();
    Lattice L{vector<char>(n, 0), vector<char>(n, 1), 0};
    while (true) {
        auto stA = state_of(prob, L.A), stB = state_of(prob, L.B);
        vector<char> A2 = L.A, B2 = L.B;
        bool changed = false;
        for (int e = 0; e < n; e++) {
            if (!L.B[e] || L.A[e]) continue;
            if (check_finite(stB.gain(e), "prune f(e|B-e)", e) >= 0) A2[e] = 1, changed = true;
            if (check_finite(stA.gain(e), "prune f(e|A)", e) < 0) B2[e] = 0, changed = true;
        }
        for (int e = 0; e < n; e++)
            if (A2[e] && !B2[e]) throw std::runtime_error("iterative prune produced A not subset of B");
        L.A = A2, L.B = B2;
        if (!changed) break;
        L.rounds++;
    }
    return L;
}

// deterministic double greedy on [A, B], elements of B \ A in index order
template <class P>
vector<char> double_greedy(const P& prob, const vector<char>& A, const vector<char>& B) {
    auto X = state_of(prob, A), Y = state_of(prob, B);
    for (int e = 0; e < prob.n(); e++) {
        if (!B[e] || A[e]) continue;
        double a = check_finite(X.gain(e), "double greedy f(e|X)", e);
        double b = -check_finite(Y.gain(e), "double greedy f(e|Y-e)", e);
        if (a >= b) X.add(e); else Y.remove(e);
    }
    return X.in_S;
}

struct MuContext {
    vector<char> A, B;
    vector<double> f_e_Bminus;  // f(e | B* - e) for e in B*
    vector<double> f_e_A;       // f(e | A*)     for e not in A*
};

template <class P>
MuContext mu_context(const P& prob, const Lattice& L) {
    int n = prob.n();
    MuContext c{L.A, L.B, vector<double>(n, NAN), vector<double>(n, NAN)};
    auto stB = state_of(prob, L.B), stA = state_of(prob, L.A);
    for (int e = 0; e < n; e++) {
        if (L.B[e]) c.f_e_Bminus[e] = check_finite(stB.gain(e), "f(e|B*-e)", e);
        if (!L.A[e]) c.f_e_A[e] = check_finite(stA.gain(e), "f(e|A*)", e);
    }
    return c;
}

// (mu2(X), mu3(X)) for A* <= X <= B*
template <class P>
std::pair<double, double> mu_bounds(const P& prob, const MuContext& c, const vector<char>& X) {
    auto st = state_of(prob, X);
    double fX = check_finite(prob.eval(X), "f(X)", -1), m2 = fX, m3 = fX;
    for (int e = 0; e < prob.n(); e++) {
        if (!c.B[e]) continue;
        if (X[e] && !c.A[e]) {
            m2 += std::max(0.0, -c.f_e_Bminus[e]);
            m3 += std::max(0.0, -check_finite(st.gain(e), "f(e|X-e)", e));
        } else if (!X[e]) {
            m2 += std::max(0.0, check_finite(st.gain(e), "f(e|X)", e));
            m3 += std::max(0.0, c.f_e_A[e]);
        }
    }
    return {m2, m3};
}

inline vector<char> project(const vector<char>& S, const vector<char>& A, const vector<char>& B) {
    vector<char> X(S.size());
    for (size_t i = 0; i < S.size(); i++) X[i] = (S[i] || A[i]) && B[i];
    return X;
}

// best f over the lattice [A, B] by size (exhaustive; small free part only)
template <class P>
vector<double> lattice_best_by_size(const P& prob, const vector<char>& A, const vector<char>& B) {
    int n = prob.n(), base = 0;
    vector<int> free_;
    for (int e = 0; e < n; e++) {
        base += A[e];
        if (B[e] && !A[e]) free_.push_back(e);
    }
    auto st = state_of(prob, A);
    vector<double> best(n + 1, -INFINITY);
    auto rec = [&](auto&& self, size_t i, int size, double cur) -> void {
        if (i == free_.size()) { best[size] = std::max(best[size], cur); return; }
        self(self, i + 1, size, cur);
        int e = free_[i];
        double g = st.gain(e);
        st.add(e);
        self(self, i + 1, size + 1, cur + g);
        st.remove(e);
    };
    rec(rec, 0, base, prob.eval(A));
    return best;
}

// LP rows for base set S:  z - sum_a coef_a x_a <= const
//   const = f(S) - sum_{a in S} f(a|V-a),  coef_a = f(a|S) (a not in S) or f(a|V-a) (a in S)
template <class P>
void lp_row(const P& prob, const vector<char>& S, const vector<double>& f_a_Vminus, double& cst,
            vector<double>& coef) {
    int n = prob.n();
    auto st = state_of(prob, S);
    cst = check_finite(prob.eval(S), "LP f(S)", -1);
    coef.assign(n, 0.0);
    for (int a = 0; a < n; a++) {
        if (S[a]) cst -= f_a_Vminus[a], coef[a] = f_a_Vminus[a];
        else coef[a] = check_finite(st.gain(a), "LP f(a|S)", a);
    }
}

// ---- B5 hybrid LP support -------------------------------------------------------------------
// The ordering, marginals and caps NM-Dual uses for base set S: the same code path as the
// prelude of dual_dp_rows (dual_core.hpp) and the same high_cap_U, so the values are identical
// (the driver checks this by rebuilding NM-Dual's DP rows from them).
struct CapData {
    vector<int> order;  // V \ S by nonincreasing f(a | S)
    vector<double> g;   // f(order[j] | S)
    vector<double> U;   // U_tilde, j = 0..m-1
};

template <class P>
CapData nm_dual_caps(const P& prob, const vector<char>& S) {
    int n = prob.n();
    typename P::State st(prob);
    for (int v = 0; v < n; v++)
        if (S[v] == 1) st.add(v);
    vector<double> single(n, 0.0);
    vector<int> order;
    for (int i = 0; i < n; i++)
        if (!S[i]) {
            order.push_back(i);
            single[i] = st.gain(i);
        }
    std::sort(order.begin(), order.end(), [&single](int a, int b) { return single[a] > single[b]; });
    CapData c;
    if (order.empty()) return c;
    c.U = high_cap_U(st, order);
    c.order = order;
    for (int a : order) c.g.push_back(check_finite(single[a], "caps f(a|S)", a));
    for (double u : c.U) check_finite(u, "cap U", -1);
    return c;
}

// the DP of dual_upper_bound evaluated from CapData (verifies that the caps are NM-Dual's)
inline vector<double> dp_rows_from_caps(const CapData& c, int kmax) {
    int m = c.order.size();
    vector<double> rows(kmax + 1, 0.0);
    if (m == 0) return rows;
    vector<double> prev(m + 1, 0.0), cur(m + 1);
    for (int j = 1; j <= kmax; j++) {
        cur[0] = -DBL_MAX;
        for (int i = 1; i <= m; i++) cur[i] = std::max(cur[i - 1], std::min(prev[i - 1] + c.g[i - 1], c.U[i - 1]));
        rows[j] = cur[m];
        std::swap(prev, cur);
    }
    return rows;
}

// exhaustive search: for each size s <= kb, the best value and one set attaining it
template <class P>
std::pair<vector<double>, vector<vector<int>>> brute_force_argmax_by_size(const P& prob, int kb) {
    typename P::State st(prob);
    vector<double> best(kb + 1, -DBL_MAX);
    vector<vector<int>> arg(kb + 1);
    vector<int> cur_set;
    auto rec = [&](auto&& self, int start, int depth, double cur) -> void {
        if (cur > best[depth]) best[depth] = cur, arg[depth] = cur_set;
        if (depth == kb) return;
        for (int v = start; v < prob.n(); v++) {
            double g = st.gain(v);
            st.add(v);
            cur_set.push_back(v);
            self(self, v + 1, depth + 1, cur + g);
            cur_set.pop_back();
            st.remove(v);
        }
    };
    rec(rec, 0, 0, prob.eval(st.in_S));
    return {best, arg};
}

// certificate P^S_i = f_S(elements of O \ S among the first i positions of the S-ordering)
template <class P>
vector<double> certificate_P(const P& prob, const vector<char>& S, const CapData& c, const vector<char>& O) {
    auto st = state_of(prob, S);
    vector<double> Pv;
    double acc = 0;
    for (int a : c.order) {
        if (O[a]) {
            acc += check_finite(st.gain(a), "certificate gain", a);
            st.add(a);
        }
        Pv.push_back(acc);
    }
    return Pv;
}

}  // namespace dual

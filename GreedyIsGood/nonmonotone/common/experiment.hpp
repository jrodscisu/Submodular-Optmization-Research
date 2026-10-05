// k-sweep driver shared by every problem: for each k in a list it reports OPT (when brute
// force fits the budget), the dual bound, the top-k singleton bound, the problem's
// "total weight" bound, plain greedy and random greedy, as one CSV row per k.
//
// Efficiency: plain greedy's prefixes S_0, S_1, ... are the same for every budget k, and the
// DP rows of dual_upper_bound do not depend on k. So each prefix S_i is evaluated once
// (f, penalty, high_cap_U, DP up to kmax) and
//     dual(k) = min_{i <= min(k, t)} f(S_i) + penalty(S_i) + max(0, max_{j<=k} dp_i[j]),
// which equals dual_wrapper(k, plain_greedy(k) chain) exactly (--check-direct asserts it).
//
// --chain-stride s > 1 only evaluates prefixes i with i % s == 0 plus the last prefix of
// every k. The minimum over fewer sets is still a valid (possibly looser) upper bound;
// use it for large instances, where high_cap_U costs O(n^2) oracle calls per prefix.
#pragma once
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "baselines.hpp"
#include "dual_core.hpp"

namespace dual {

// --key value command-line arguments
struct Args {
    std::map<std::string, std::string> kv;
    Args(int argc, char** argv) {
        for (int i = 1; i < argc; i++) {
            std::string a = argv[i];
            if (a.rfind("--", 0) != 0) throw std::runtime_error("unexpected argument: " + a);
            std::string key = a.substr(2);
            if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) kv[key] = argv[++i];
            else kv[key] = "1";
        }
    }
    bool has(const std::string& k) const { return kv.count(k) > 0; }
    std::string str(const std::string& k, const std::string& d = "") const { return has(k) ? kv.at(k) : d; }
    double num(const std::string& k, double d) const { return has(k) ? atof(kv.at(k).c_str()) : d; }
    long long integer(const std::string& k, long long d) const { return has(k) ? atoll(kv.at(k).c_str()) : d; }
};

// comma-separated values and ranges a:b[:step], e.g. "1:20", "1,10:200:10";
// values are clipped to [1, n]
inline vector<int> parse_ks(const std::string& spec, int n) {
    vector<int> ks;
    std::istringstream all(spec);
    std::string tok;
    while (std::getline(all, tok, ',')) {
        if (tok.find(':') == std::string::npos) { ks.push_back(atoi(tok.c_str())); continue; }
        int a = 1, b = n, s = 1;
        char c;
        std::istringstream in(tok);
        in >> a >> c >> b;
        if (in >> c) in >> s;
        for (int k = a; k <= b; k += std::max(1, s)) ks.push_back(k);
    }
    vector<int> out;
    for (int k : ks) if (k >= 1 && k <= n) out.push_back(k);
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

struct SweepConfig {
    std::string problem, instance;
    vector<int> ks;
    int trials = 200;
    unsigned long long seed = 0;
    std::string csv, prefix_csv;
    double brute_budget = 2e6;  // max #subsets enumerated for OPT
    int chain_stride = 1;
    bool check_direct = false;
    bool check_oracle = false;
    bool baselines = false;     // B2-B4 columns (common/baselines.hpp)
    bool monotone = false;      // M1/M2/A2 columns of the violations study
    bool monotone_only = false; // only the monotone methods (no NM-Dual, brute force or random greedy)
    std::string lp_export;      // directory for the B2 LP coefficients
    double total_bound = NAN;   // problem-specific "sum of all weights" bound (nan if none)
};

inline SweepConfig sweep_config(const Args& a, int n, const std::string& problem, const std::string& instance,
                                double total_bound) {
    SweepConfig c;
    c.problem = problem;
    c.instance = a.str("instance", instance);
    c.ks = parse_ks(a.str("ks", "1:" + std::to_string(std::min(n, 20))), n);
    c.trials = a.integer("trials", 200);
    c.seed = a.integer("seed", 0);
    c.csv = a.str("out", "results.csv");
    c.prefix_csv = a.str("prefix-out", "");
    c.brute_budget = a.num("brute-budget", 2e6);
    c.chain_stride = std::max<long long>(1, a.integer("chain-stride", 1));
    c.check_direct = a.has("check-direct");
    c.check_oracle = a.has("check-oracle");
    c.baselines = a.has("baselines");
    c.monotone = a.has("monotone");
    c.monotone_only = a.has("monotone-only");
    c.lp_export = a.str("lp-export", "");
    c.total_bound = total_bound;
    return c;
}

inline double ms_since(std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
}

inline FILE* open_csv(const std::string& path, const char* header) {
    FILE* f = fopen(path.c_str(), "a+");
    if (!f) { perror(path.c_str()); exit(1); }
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) fputs(header, f);
    return f;
}

// number of subsets of size <= k of an n-set, saturating at 1e300
inline double subsets_up_to(int n, int k) {
    double total = 0, c = 1;
    for (int s = 0; s <= k; s++) {
        total += c;
        c = c * (n - s) / (s + 1);
    }
    return total;
}

// Verifies the incremental oracle: on random sets S (built by add/remove sequences),
// State::gain(v) must equal eval(S + v) - eval(S - v) for every v, in and out of S.
template <class P>
void check_oracle(const P& prob, unsigned long long seed, int rounds = 20) {
    int n = prob.n();
    std::mt19937_64 rng(seed + 99);
    typename P::State st(prob);
    double worst = 0;
    for (int r = 0; r < rounds; r++) {
        for (int step = 0; step < 3 * n; step++) {  // random walk over sets
            int v = rng() % n;
            if (st.in_S[v]) st.remove(v); else if (rng() % 2) st.add(v);
        }
        vector<char> S = st.in_S;
        for (int v = 0; v < n; v++) {
            vector<char> plus = S, minus = S;
            plus[v] = 1, minus[v] = 0;
            double expect = prob.eval(plus) - prob.eval(minus), got = st.gain(v);
            double err = std::fabs(got - expect) / std::max(1.0, std::fabs(expect));
            worst = std::max(worst, err);
            if (!(err < 1e-6)) {
                fprintf(stderr, "check-oracle FAILED: |S|=%d v=%d in_S=%d gain=%.10g expected=%.10g\n",
                        (int)std::count(S.begin(), S.end(), 1), v, (int)S[v], got, expect);
                exit(3);
            }
        }
    }
    printf("  oracle check passed (%d random sets, max rel err %.2e)\n", rounds, worst);
}

template <class P>
void run_k_sweep(const P& prob, const SweepConfig& cfg) {
    using clock = std::chrono::steady_clock;
    int n = prob.n();
    if (cfg.ks.empty()) { fprintf(stderr, "no valid k values\n"); exit(1); }
    int kmax = cfg.ks.back();
    printf("[%s/%s] n=%d, k in [%d, %d] (%zu values), seed=%llu, stride=%d\n", cfg.problem.c_str(),
           cfg.instance.c_str(), n, cfg.ks.front(), kmax, cfg.ks.size(), cfg.seed, cfg.chain_stride);
    fflush(stdout);

    if (getenv("GREEDY_LENGTH_ONLY")) {  // Phase-3 estimate: length of the unbudgeted greedy chain
        auto [gv, ch] = plain_greedy(prob, n);
        if (ch.size() >= 2 && ch.back() == ch[ch.size() - 2]) ch.pop_back();
        printf("GREEDY_LENGTH %d\n", (int)ch.size() - 1);
        return;
    }
    if (cfg.check_oracle) check_oracle(prob, cfg.seed);
    vector<double> single = singleton_gains(prob);

    // plain greedy chain for the largest budget; prefixes are shared by every k
    auto greedy_run = plain_greedy(prob, kmax);
    auto& chain = greedy_run.second;
    if (chain.size() >= 2 && chain.back() == chain[chain.size() - 2]) chain.pop_back();
    int t = chain.size() - 1;  // greedy stops after t additions
    vector<double> f_chain(chain.size());
    for (size_t i = 0; i < chain.size(); i++) f_chain[i] = prob.eval(chain[i]);

    // which prefixes need the dual: stride multiples and the last prefix of every k
    vector<char> need(chain.size(), 0);
    for (int i = 0; i <= t; i += cfg.chain_stride) need[i] = 1;
    for (int k : cfg.ks) need[std::min(k, t)] = 1;

    vector<double> pen(chain.size(), NAN), cost(chain.size(), 0.0);
    vector<vector<double>> rows(chain.size());
    double time_prefix_total = 0;

    // ---- published monotone methods (violations study; --monotone / --monotone-only). New code
    // path only: M1 (BQS Dual), M2 (Marginal without penalty), A2 (our DP, raw caps f_S(A_i), with
    // penalty) over exactly the base sets NM-Dual uses at each k; f_S(A_i) is exported for M3.
    auto uses_k = [&](int i, int k) {
        int last = std::min(k, t);
        return i <= last && need[i] && (i % cfg.chain_stride == 0 || i == last);
    };
    struct {
        vector<MonoPrefix> pre;
        double min_h = NAN, t_mono = 0;
        int flag = -1;  // 1 iff f(a | V - a) >= 0 for every a (f monotone)
    } mo;
    auto compute_mono = [&]() {
        auto t0 = clock::now();
        vector<char> all(n, 1);
        auto stV = state_of(prob, all);
        double mh = INFINITY, scale = 1.0;
        for (int a = 0; a < n; a++) {
            double h = check_finite(stV.gain(a), "f(a|V-a)", a);
            mh = std::min(mh, h), scale = std::max(scale, std::fabs(h));
        }
        mo.min_h = mh;
        mo.flag = mh >= -1e-9 * scale;
        mo.pre.assign(chain.size(), {});
        for (int i = 0; i <= t; i++)
            if (need[i]) mo.pre[i] = monotone_prefix(prob, chain[i], kmax);
        if (!cfg.lp_export.empty()) {  // O <prefix> <order>, F <prefix> <f_S(A_1..A_m)>
            std::string path = cfg.lp_export + "/" + cfg.instance + "__seed" + std::to_string(cfg.seed) + ".mono.txt";
            FILE* f = fopen(path.c_str(), "w");
            if (!f) { perror(path.c_str()); exit(1); }
            fprintf(f, "# monotone prefix caps for M3: O <prefix> <order>; F <prefix> <f_S(A_i), i=1..m>\nn %d\n", n);
            for (int i = 0; i <= t; i++) {
                if (!need[i]) continue;
                fprintf(f, "O %d", i);
                for (int a : mo.pre[i].order) fprintf(f, " %d", a);
                fprintf(f, "\nF %d", i);
                for (double v : mo.pre[i].F) fprintf(f, " %.17g", v);
                fputc('\n', f);
            }
            fclose(f);
        }
        mo.t_mono = ms_since(t0);
    };
    auto mono_bounds = [&](int k, double& m1, double& m2, double& a2) {
        m1 = m2 = a2 = INFINITY;
        for (int i = 0; i <= t; i++) {
            if (!uses_k(i, k)) continue;
            const MonoPrefix& p = mo.pre[i];
            m1 = std::min(m1, f_chain[i] + p.V[k]);
            m2 = std::min(m2, f_chain[i] + p.m2top[k]);
            a2 = std::min(a2, f_chain[i] + pen[i] + dual_from_rows(p.a2rows, k));
        }
    };
    if (cfg.monotone_only) {  // no NM-Dual, no brute force, no random greedy
        for (int i = 0; i <= t; i++)
            if (need[i]) pen[i] = penalty_S(prob, chain[i]);
        compute_mono();
        FILE* csv = open_csv(cfg.csv, "problem,instance,n,k,seed,chain_stride,chain_len,greedy,m1_bound,m2_bound,"
                                      "a2_bound,monotone_flag,min_f_a_V_minus_a,time_monotone_ms\n");
        for (int k : cfg.ks) {
            double m1, m2, a2;
            mono_bounds(k, m1, m2, a2);
            int last = std::min(k, t);
            fprintf(csv, "%s,%s,%d,%d,%llu,%d,%d,%.10g,%.10g,%.10g,%.10g,%d,%.10g,%.3f\n", cfg.problem.c_str(),
                    cfg.instance.c_str(), n, k, cfg.seed, cfg.chain_stride, last + 1, f_chain[last], m1, m2, a2,
                    mo.flag, mo.min_h, mo.t_mono);
        }
        fclose(csv);
        printf("  monotone methods: flag=%d min f(a|V-a)=%.6g (%.0f ms)\n", mo.flag, mo.min_h, mo.t_mono);
        return;
    }
    for (int i = 0; i <= t; i++) {
        if (!need[i]) continue;
        auto t0 = clock::now();
        pen[i] = penalty_S(prob, chain[i]);
        rows[i] = dual_dp_rows(prob, kmax, chain[i]);
        cost[i] = ms_since(t0);
        time_prefix_total += cost[i];
        printf("  prefix %d/%d: f=%.4f penalty=%.4f dual(kmax)=%.4f  (%.0f ms)\n", i, t, f_chain[i], pen[i],
               dual_from_rows(rows[i], kmax), cost[i]);
        fflush(stdout);
    }

    // OPT by brute force for the k whose subset count fits the budget
    int kb = 0;
    while (kb < kmax && subsets_up_to(n, kb + 1) <= cfg.brute_budget) kb++;
    vector<double> best_by_size;
    double time_opt = NAN;
    if (kb >= cfg.ks.front()) {
        auto t0 = clock::now();
        best_by_size = brute_force_by_size(prob, kb);
        time_opt = ms_since(t0);
    }

    // ---- baselines B2-B4 (only with --baselines); NM-Dual above is unchanged
    struct {
        double f_empty = NAN, f_V = NAN, gamma1 = NAN, gamma1_np = NAN, fixed_mu2 = INFINITY, fixed_mu3 = INFINITY;
        double opt_unc = NAN, lat_opt_unc = NAN, t_gamma = 0, t_mu = 0, t_lp = 0;
        int A_size = 0, B_size = 0, rounds = 0;
        vector<double> mu2, mu3, lat_best;
        vector<vector<double>> marg_top;  // marg_top[i][j] = sum of the j largest [f(a | S_i)]^+, a not in S_i
        double t_marg = 0, t_hybrid = 0;
    } bl;
    auto uses_prefix = [&](int i, int k) {
        int last = std::min(k, t);
        return i <= last && need[i] && (i % cfg.chain_stride == 0 || i == last);
    };
    if (cfg.baselines) {
        vector<char> none(n, 0), all(n, 1);
        bl.f_empty = check_finite(prob.eval(none), "f(empty)", -1);
        bl.f_V = check_finite(prob.eval(all), "f(V)", -1);

        auto t0 = clock::now();
        Lattice L = iterative_prune(prob);
        vector<char> SD = double_greedy(prob, L.A, L.B), SD0 = double_greedy(prob, none, all);
        bl.gamma1 = 3 * prob.eval(SD) - prob.eval(L.A) - prob.eval(L.B);
        bl.gamma1_np = 3 * prob.eval(SD0) - bl.f_empty - bl.f_V;
        bl.t_gamma = ms_since(t0);
        bl.rounds = L.rounds;
        for (int e = 0; e < n; e++) bl.A_size += L.A[e], bl.B_size += L.B[e];

        t0 = clock::now();
        MuContext ctx = mu_context(prob, L);
        vector<vector<char>> fixed{L.A, L.B, SD};
        std::mt19937_64 lat_rng(20261004ULL);  // fixed seed for the 5 random lattice sets
        for (int r = 0; r < 5; r++) {
            vector<char> X = L.A;
            for (int e = 0; e < n; e++) if (L.B[e] && !L.A[e]) X[e] = lat_rng() & 1;
            fixed.push_back(X);
        }
        for (auto& X : fixed) {
            auto [m2, m3] = mu_bounds(prob, ctx, X);
            bl.fixed_mu2 = std::min(bl.fixed_mu2, m2), bl.fixed_mu3 = std::min(bl.fixed_mu3, m3);
        }
        bl.mu2.assign(chain.size(), NAN), bl.mu3.assign(chain.size(), NAN);
        for (int i = 0; i <= t; i++)
            if (need[i]) std::tie(bl.mu2[i], bl.mu3[i]) = mu_bounds(prob, ctx, project(chain[i], L.A, L.B));
        bl.t_mu = ms_since(t0);

        // Marginal bound: min_S f(S) + Pen(S) + sum of the top-k [f(a | S)]^+ over the dual's base sets
        // (= NM-Dual without the high_cap_U caps); Pen(S) = penalty_S, already computed in pen[i]
        t0 = clock::now();
        bl.marg_top.assign(chain.size(), {});
        for (int i = 0; i <= t; i++) {
            if (!need[i]) continue;
            auto st = state_of(prob, chain[i]);
            vector<double> g;
            for (int a = 0; a < n; a++)
                if (!chain[i][a]) {
                    double v = check_finite(st.gain(a), "marginal f(a|S)", a);
                    if (v > 0) g.push_back(v);
                }
            std::sort(g.begin(), g.end(), std::greater<double>());
            vector<double>& top = bl.marg_top[i];
            top.assign(kmax + 1, 0.0);
            for (int j = 1; j <= kmax; j++) top[j] = top[j - 1] + (j <= (int)g.size() ? g[j - 1] : 0.0);
        }
        bl.t_marg = ms_since(t0);

        // B2: LP rows for every base set NM-Dual uses, and which rows each k uses
        if (!cfg.lp_export.empty()) {
            t0 = clock::now();
            auto stV = state_of(prob, all);
            vector<double> fVm(n);
            for (int a = 0; a < n; a++) fVm[a] = check_finite(stV.gain(a), "f(a|V-a)", a);
            std::string path = cfg.lp_export + "/" + cfg.instance + "__seed" + std::to_string(cfg.seed) + ".lp.txt";
            FILE* lp = fopen(path.c_str(), "w");
            if (!lp) { perror(path.c_str()); exit(1); }
            fprintf(lp, "# B2 LP rows: P <prefix> <const> <coef_0..coef_n-1>;  K <k> <prefixes used>\nn %d\n", n);
            vector<double> coef;
            double cst;
            for (int i = 0; i <= t; i++) {
                if (!need[i]) continue;
                lp_row(prob, chain[i], fVm, cst, coef);
                fprintf(lp, "P %d %.17g", i, cst);
                for (double c : coef) fprintf(lp, " %.17g", c);
                fputc('\n', lp);
            }
            for (int k : cfg.ks) {
                fprintf(lp, "K %d", k);
                for (int i = 0; i <= t; i++) if (uses_prefix(i, k)) fprintf(lp, " %d", i);
                fputc('\n', lp);
            }
            fclose(lp);
            bl.t_lp = ms_since(t0);

            // B5 hybrid LP data: for every base set NM-Dual's ordering, marginals and caps (recomputed
            // with the same code and checked against NM-Dual's DP rows); where brute force ran, an
            // optimal set per k and the certificate vectors P of the validity proof (check 3)
            t0 = clock::now();
            std::string hpath = cfg.lp_export + "/" + cfg.instance + "__seed" + std::to_string(cfg.seed) + ".hybrid.txt";
            FILE* hy = fopen(hpath.c_str(), "w");
            if (!hy) { perror(hpath.c_str()); exit(1); }
            fprintf(hy, "# B5 hybrid LP data. V: f(a|V-a); per base set: H <prefix> <f(S)> <m>, M <members>, "
                        "O <order>, G <f(a|S) in order>, U <caps>; K <k> <prefixes>; Q <k> <f(O)> <O>; "
                        "C <k> <prefix> <P_1..P_m>\nn %d\nV", n);
            for (double v : fVm) fprintf(hy, " %.17g", v);
            fputc('\n', hy);
            vector<CapData> caps(chain.size());
            for (int i = 0; i <= t; i++) {
                if (!need[i]) continue;
                caps[i] = nm_dual_caps(prob, chain[i]);
                vector<double> chk = dp_rows_from_caps(caps[i], kmax);
                for (int j = 0; j <= kmax; j++)
                    if (!(chk[j] == rows[i][j])) {
                        fprintf(stderr, "CAPS MISMATCH [%s/%s seed %llu] prefix %d row %d: %.17g vs NM-Dual %.17g\n",
                                cfg.problem.c_str(), cfg.instance.c_str(), cfg.seed, i, j, chk[j], rows[i][j]);
                        exit(4);
                    }
                fprintf(hy, "H %d %.17g %d\nM", i, f_chain[i], (int)caps[i].order.size());
                for (int a = 0; a < n; a++) if (chain[i][a]) fprintf(hy, " %d", a);
                fprintf(hy, "\nO");
                for (int a : caps[i].order) fprintf(hy, " %d", a);
                fprintf(hy, "\nG");
                for (double v : caps[i].g) fprintf(hy, " %.17g", v);
                fprintf(hy, "\nU");
                for (double v : caps[i].U) fprintf(hy, " %.17g", v);
                fputc('\n', hy);
            }
            for (int k : cfg.ks) {
                fprintf(hy, "K %d", k);
                for (int i = 0; i <= t; i++) if (uses_prefix(i, k)) fprintf(hy, " %d", i);
                fputc('\n', hy);
            }
            if (kb >= cfg.ks.front()) {
                auto [bv, bs] = brute_force_argmax_by_size(prob, kb);
                for (int k : cfg.ks) {
                    if (k > kb) break;
                    int sb = 0;
                    for (int s = 1; s <= k; s++) if (bv[s] > bv[sb]) sb = s;
                    vector<char> O(n, 0);
                    for (int a : bs[sb]) O[a] = 1;
                    fprintf(hy, "Q %d %.17g", k, bv[sb]);
                    for (int a : bs[sb]) fprintf(hy, " %d", a);
                    fputc('\n', hy);
                    for (int i = 0; i <= t; i++) {
                        if (!uses_prefix(i, k)) continue;
                        fprintf(hy, "C %d %d", k, i);
                        for (double v : certificate_P(prob, chain[i], caps[i], O)) fprintf(hy, " %.17g", v);
                        fputc('\n', hy);
                    }
                }
            }
            fclose(hy);
            bl.t_hybrid = ms_since(t0);
        }

        // lattice contains an unconstrained optimum? (only when every subset was enumerated)
        int free_ = bl.B_size - bl.A_size;
        if (kb == n && free_ <= 24) {
            bl.opt_unc = -INFINITY;
            for (double b : best_by_size) bl.opt_unc = std::max(bl.opt_unc, b);
            bl.lat_best = lattice_best_by_size(prob, L.A, L.B);
            bl.lat_opt_unc = -INFINITY;
            for (double b : bl.lat_best) bl.lat_opt_unc = std::max(bl.lat_opt_unc, b);
        }
        printf("  baselines: |A*|=%d |B*|=%d (%d prune rounds) gamma1=%.4f gamma1_noprune=%.4f "
               "opt_unc=%.4f lattice_opt_unc=%.4f  (gamma %.0f ms, mu %.0f ms, lp export %.0f ms)\n",
               bl.A_size, bl.B_size, bl.rounds, bl.gamma1, bl.gamma1_np, bl.opt_unc, bl.lat_opt_unc,
               bl.t_gamma, bl.t_mu, bl.t_lp);
        fflush(stdout);
    }

    std::string header =
        "problem,instance,n,k,trials,seed,chain_stride,chain_len,opt,top_k_bound,total_bound,dual_bound,"
        "dual_bound_S0,dual_best_prefix,greedy,rg_mean,rg_std,rg_min,rg_max,best_found,dual_valid,"
        "time_dual_ms,time_rg_ms,time_opt_ms";
    if (cfg.baselines)
        header += ",lp_bound,gamma1_bound,gamma1_noprune,mu2_bound,mu3_bound,lattice_A_size,lattice_B_size,"
                  "prune_rounds,f_empty,f_V,opt_unconstrained,lattice_opt_unconstrained,lattice_opt_k,"
                  "time_lp_export_ms,time_lp_ms,time_gamma1_ms,time_mu_ms,marginal_bound,time_marginal_ms,"
                  "time_hybrid_export_ms";
    if (cfg.monotone) {
        compute_mono();
        header += ",m1_bound,m2_bound,a2_bound,monotone_flag,min_f_a_V_minus_a,time_monotone_ms";
    }
    FILE* csv = open_csv(cfg.csv, (header + "\n").c_str());
    FILE* pcsv = cfg.prefix_csv.empty() ? nullptr
        : open_csv(cfg.prefix_csv, "problem,instance,seed,k,prefix,size,f_S,penalty_S,dual_S,bound_S\n");

    for (int k : cfg.ks) {
        int last = std::min(k, t);
        double dual_bound = DBL_MAX, dual_S0 = NAN, time_dual = 0;
        int best_prefix = -1;
        for (int i = 0; i <= last; i++) {
            if (!need[i] || (i % cfg.chain_stride != 0 && i != last)) continue;
            double d = dual_from_rows(rows[i], k), b = f_chain[i] + pen[i] + d;
            if (i == 0) dual_S0 = b;
            if (b < dual_bound) dual_bound = b, best_prefix = i;
            time_dual += cost[i];
            if (pcsv)
                fprintf(pcsv, "%s,%s,%llu,%d,%d,%d,%.10g,%.10g,%.10g,%.10g\n", cfg.problem.c_str(),
                        cfg.instance.c_str(), cfg.seed, k, i, i, f_chain[i], pen[i], d, b);
        }
        if (cfg.check_direct) {
            auto [gk, Sk] = plain_greedy(prob, k);
            double direct = dual_wrapper(prob, k, Sk);
            if (cfg.chain_stride == 1 && std::fabs(direct - dual_bound) > 1e-7 * std::max(1.0, std::fabs(direct))) {
                fprintf(stderr, "check-direct FAILED at k=%d: sweep %.10g vs direct %.10g\n", k, dual_bound, direct);
                exit(2);
            }
        }

        double opt = NAN;
        if (k <= kb) {
            opt = 0;
            for (int s = 0; s <= k; s++) opt = std::max(opt, best_by_size[s]);
        }

        double greedy = f_chain[last];

        std::mt19937_64 rng(cfg.seed * 1000003ULL + k);
        auto t0 = clock::now();
        double sum = 0, sum_sq = 0, mn = DBL_MAX, mx = -DBL_MAX;
        for (int r = 0; r < cfg.trials; r++) {
            double v = random_greedy(prob, k, rng);
            sum += v, sum_sq += v * v, mn = std::min(mn, v), mx = std::max(mx, v);
        }
        double time_rg = cfg.trials ? ms_since(t0) / cfg.trials : NAN;
        double mean = cfg.trials ? sum / cfg.trials : NAN;
        double sd = cfg.trials ? std::sqrt(std::max(0.0, sum_sq / cfg.trials - mean * mean)) : NAN;
        if (!cfg.trials) mn = mx = NAN;

        double best_found = greedy;
        if (cfg.trials) best_found = std::max(best_found, mx);
        if (!std::isnan(opt)) best_found = std::max(best_found, opt);
        double tol = 1e-7 * std::max(1.0, std::fabs(best_found));
        int dual_valid = dual_bound >= best_found - tol;
        if (!dual_valid)
            fprintf(stderr, "WARNING [%s/%s] k=%d: dual bound %.6f < best solution %.6f\n", cfg.problem.c_str(),
                    cfg.instance.c_str(), k, dual_bound, best_found);

        double topk = top_k_upper_bound(single, k);
        printf("  k=%-4d opt=%-10.4f dual=%-10.4f top_k=%-10.4f total=%-10.4f greedy=%-10.4f rg=%-10.4f "
               "greedy/dual=%.3f\n", k, opt, dual_bound, topk, cfg.total_bound, greedy, mean, greedy / dual_bound);
        fflush(stdout);

        fprintf(csv, "%s,%s,%d,%d,%d,%llu,%d,%d,%.10g,%.10g,%.10g,%.10g,%.10g,%d,%.10g,%.10g,%.10g,%.10g,%.10g,"
                     "%.10g,%d,%.3f,%.4f,%.3f",
                cfg.problem.c_str(), cfg.instance.c_str(), n, k, cfg.trials, cfg.seed, cfg.chain_stride, last + 1,
                opt, topk, cfg.total_bound, dual_bound, dual_S0, best_prefix, greedy, mean, sd, mn, mx, best_found,
                dual_valid, time_dual, time_rg, time_opt);
        if (cfg.baselines) {
            double mu2 = bl.fixed_mu2, mu3 = bl.fixed_mu3, lat_k = NAN, marg = INFINITY;
            for (int i = 0; i <= t; i++)
                if (uses_prefix(i, k)) {
                    mu2 = std::min(mu2, bl.mu2[i]), mu3 = std::min(mu3, bl.mu3[i]);
                    marg = std::min(marg, f_chain[i] + pen[i] + bl.marg_top[i][k]);
                }
            // theorems: NM-Dual <= Marginal (caps only lower each term) and Marginal <= top-k (S_0 = {})
            double tolm = 1e-7 * std::max(1.0, std::fabs(marg));
            if (!(dual_bound <= marg + tolm) || !(marg <= topk + tolm + 1e-7 * std::fabs(bl.f_empty)))
                fprintf(stderr, "ORDER VIOLATION [%s/%s seed %llu] k=%d: dual=%.10g marginal=%.10g top_k=%.10g\n",
                        cfg.problem.c_str(), cfg.instance.c_str(), cfg.seed, k, dual_bound, marg, topk);
            if (!bl.lat_best.empty()) {
                lat_k = -INFINITY;
                for (int s = 0; s <= k; s++) lat_k = std::max(lat_k, bl.lat_best[s]);
            }
            if (!std::isnan(opt)) {
                double tolb = 1e-6 * std::max(1.0, std::fabs(opt));
                const char* names[] = {"gamma1_bound", "gamma1_noprune", "mu2_bound", "mu3_bound", "marginal_bound"};
                double vals[] = {bl.gamma1, bl.gamma1_np, mu2, mu3, marg};
                for (int j = 0; j < 5; j++)
                    if (!(vals[j] >= opt - tolb))
                        fprintf(stderr, "BASELINE VIOLATION [%s/%s seed %llu] k=%d: %s=%.10g < OPT=%.10g\n",
                                cfg.problem.c_str(), cfg.instance.c_str(), cfg.seed, k, names[j], vals[j], opt);
            }
            fprintf(csv, ",nan,%.10g,%.10g,%.10g,%.10g,%d,%d,%d,%.10g,%.10g,%.10g,%.10g,%.10g,%.3f,nan,%.3f,%.3f",
                    bl.gamma1, bl.gamma1_np, mu2, mu3, bl.A_size, bl.B_size, bl.rounds, bl.f_empty, bl.f_V,
                    bl.opt_unc, bl.lat_opt_unc, lat_k, bl.t_lp, bl.t_gamma, bl.t_mu);
            fprintf(csv, ",%.10g,%.3f,%.3f", marg, bl.t_marg, bl.t_hybrid);
        }
        if (cfg.monotone) {
            double m1, m2, a2;
            mono_bounds(k, m1, m2, a2);
            fprintf(csv, ",%.10g,%.10g,%.10g,%d,%.10g,%.3f", m1, m2, a2, mo.flag, mo.min_h, mo.t_mono);
        }
        fputc('\n', csv);
        fflush(csv);
    }
    fclose(csv);
    if (pcsv) fclose(pcsv);
    printf("  total prefix time %.0f ms\n", time_prefix_total);
}

}  // namespace dual

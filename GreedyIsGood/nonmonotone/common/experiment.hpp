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

    if (cfg.check_oracle) check_oracle(prob, cfg.seed);
    vector<double> single = singleton_gains(prob);

    // plain greedy chain for the largest budget; prefixes are shared by every k
    auto [g_final, chain] = plain_greedy(prob, kmax);
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

    FILE* csv = open_csv(cfg.csv,
        "problem,instance,n,k,trials,seed,chain_stride,chain_len,opt,top_k_bound,total_bound,dual_bound,"
        "dual_bound_S0,dual_best_prefix,greedy,rg_mean,rg_std,rg_min,rg_max,best_found,dual_valid,"
        "time_dual_ms,time_rg_ms,time_opt_ms\n");
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
                     "%.10g,%d,%.3f,%.4f,%.3f\n",
                cfg.problem.c_str(), cfg.instance.c_str(), n, k, cfg.trials, cfg.seed, cfg.chain_stride, last + 1,
                opt, topk, cfg.total_bound, dual_bound, dual_S0, best_prefix, greedy, mean, sd, mn, mx, best_found,
                dual_valid, time_dual, time_rg, time_opt);
        fflush(csv);
    }
    fclose(csv);
    if (pcsv) fclose(pcsv);
    printf("  total prefix time %.0f ms\n", time_prefix_total);
}

}  // namespace dual

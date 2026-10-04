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
#include "maxcut_problem.hpp"

using namespace std;
using namespace dual;

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
        MaxCut P{dual::synthetic_graph(n, p, rng)};

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

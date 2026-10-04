// Max-cut k sweep through the generic driver (common/experiment.hpp), on exactly the graphs of
// maxcut_random_greedy.cpp (same generator and seed). Used to compute the B2-B4 baselines for
// the original max-cut experiment; common/baselines_lp.py merges them onto the rows of
// results/k_sweep_n20_p0.3.csv after checking that OPT, top-k, greedy and the dual bound agree.
//
// Build: g++ -O2 -std=c++17 maxcut_baselines.cpp -o maxcut_baselines
// Run:   ./maxcut_baselines --n 20 --p 0.3 --seed 0 --ks 1:20 --trials 0 --baselines --lp-export DIR --out r.csv
#include "../common/experiment.hpp"
#include "../common/maxcut_problem.hpp"

using namespace std;
using namespace dual;

int main(int argc, char** argv) {
    Args a(argc, argv);
    int n = a.integer("n", 20);
    double p = a.num("p", 0.3);
    mt19937_64 rng(a.integer("seed", 0));
    MaxCut P{synthetic_graph(n, p, rng)};
    double total = 0;
    for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) total += P.W[i][j];
    string name = "synthetic_n" + to_string(n) + "_p" + a.str("p", "0.3");
    run_k_sweep(P, sweep_config(a, n, "max_cut", name, total));
}

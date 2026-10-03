// Edge-list loading shared by the graph problems.
// File format: one edge per line "u v [w]" (whitespace separated; lines starting with '#' or
// '%' are skipped). Node ids are arbitrary integers and are remapped to 0..n-1 in order of
// first appearance. Self-loops are dropped and parallel edges are merged (weights summed).
#pragma once
#include <cstdio>
#include <fstream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace dual {

struct Edge {
    int u, v;
    double w;
};

struct EdgeList {
    int n = 0;
    std::vector<Edge> edges;
};

inline EdgeList read_edge_list(const std::string& path, bool directed) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path);
    std::unordered_map<long long, int> id;
    std::map<std::pair<int, int>, double> w;
    auto node = [&](long long x) {
        auto it = id.find(x);
        if (it != id.end()) return it->second;
        int k = id.size();
        id[x] = k;
        return k;
    };
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '%') continue;
        std::istringstream ss(line);
        long long a, b;
        double wt = 1.0;
        if (!(ss >> a >> b)) continue;
        ss >> wt;
        int u = node(a), v = node(b);
        if (u == v) continue;
        if (!directed && u > v) std::swap(u, v);
        w[{u, v}] += wt;
    }
    EdgeList g;
    g.n = id.size();
    for (auto& [uv, wt] : w) g.edges.push_back({uv.first, uv.second, wt});
    return g;
}

// replace every weight by an independent draw (seeded): "unit", "uniform01", "int1-9"
inline void reweight(EdgeList& g, const std::string& scheme, unsigned long long seed) {
    if (scheme.empty() || scheme == "file") return;
    std::mt19937_64 rng(seed * 7919ULL + 17);
    std::uniform_real_distribution<double> U(0, 1);
    std::uniform_int_distribution<int> I(1, 9);
    for (auto& e : g.edges) {
        if (scheme == "unit") e.w = 1.0;
        else if (scheme == "uniform01") e.w = U(rng);
        else if (scheme == "int1-9") e.w = I(rng);
        else throw std::runtime_error("unknown weight scheme " + scheme);
    }
}

// G(n, p) with integer weights 1..9 (the max-cut generator); directed graphs get a random
// orientation per edge
inline EdgeList gnp_graph(int n, double p, bool directed, unsigned long long seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> U(0, 1);
    std::uniform_int_distribution<int> wt(1, 9);
    EdgeList g;
    g.n = n;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (U(rng) < p) {
                double w = wt(rng);
                if (directed && U(rng) < 0.5) g.edges.push_back({j, i, w});
                else g.edges.push_back({i, j, w});
            }
    return g;
}

}  // namespace dual

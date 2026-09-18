#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <cstring>
#include <vector>


using namespace std;

void write_bincode_vector(const std::string& out_path, const std::vector<double>& values) {
    std::ofstream out(out_path, std::ios::binary | std::ios::out);
    if (!out) {
        throw std::runtime_error("Unable to open output bin file: " + out_path);
    }

    // bincode encodes a Vec<T> as: u64 length in little-endian, followed by each item.
    const uint64_t len = static_cast<uint64_t>(values.size());
    for (int i = 0; i < 8; ++i) {
        const uint8_t byte = static_cast<uint8_t>((len >> (8 * i)) & 0xFFu);
        out.put(static_cast<char>(byte));
    }

    for (double value : values) {
        uint64_t bits = 0;
        static_assert(sizeof(double) == sizeof(uint64_t), "double must be 8 bytes");
        std::memcpy(&bits, &value, sizeof(bits));
        for (int i = 0; i < 8; ++i) {
            const uint8_t byte = static_cast<uint8_t>((bits >> (8 * i)) & 0xFFu);
            out.put(static_cast<char>(byte));
        }
    }
}



void write_txt_costs(const std::string& out_path, const std::vector<double>& values) {
    std::ofstream txt(out_path);
    if (!txt) {
        throw std::runtime_error("Unable to open output txt file: " + out_path);
    }

    txt << std::setprecision(17);
    for (double value : values) {
        txt << value << '\n';
    }
}

std::vector<double> constant_costs(std::size_t n, double value = 1.0) {
    return std::vector<double>(n, value);
}

std::vector<double> get_random_costs(std::size_t n, double min_cost, double max_cost) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(min_cost, max_cost);

    std::vector<double> costs(n);
    for (std::size_t i = 0; i < n; ++i) {
        costs[i] = dist(gen);
    }
    return costs;
}

vector<double> normalized_linear_costs(const vector<int> & d_out) {
    double edges = 0.0;
    double n = (double)d_out.size();
    for (int degree : d_out) {
        edges += degree;
    }
    vector<double> normalized(d_out.size());
    for (size_t i = 0; i < d_out.size(); ++i) {
        normalized[i] = n * d_out[i] / edges;
    }
    return normalized;
}

vector<double> normalized_logarithmic_costs(const vector<int> & d_out) {
    double edges = 0.0;
    double n = (double)d_out.size();
    for (int degree : d_out) {
        edges += log(degree);
    }
    vector<double> normalized(d_out.size());
    for (size_t i = 0; i < d_out.size(); ++i) {
        normalized[i] = n * log(d_out[i]) / edges;
    }
    return normalized;
}

vector<int> get_degrees_from_file(const string &filename) {
    ifstream infile(filename);
    int n, m;
    infile >> n >> m; // Read number of nodes and edges

    int trash;
    vector<int> degrees(n, 0);
    for (int i = 0; i < m; ++i) {
        int u, v;
        infile >> u >> v >> trash; // Read each edge
        degrees[u]++;
        degrees[v]++;
    }
    return degrees;
}

vector<double> output_graph_with_costs(const string &filename, const vector<double> &costs) {
    ifstream infile(filename);
    int n, m;
    infile >> n >> m; // Read number of nodes and edges

    ofstream outfile(filename + "with_costs.txt");
    outfile << n << " " << m << "\n";
    for (int i = 0; i < n; ++i) {
        outfile << costs[i] << "\n"; // Write costs for each node
    }

    int trash;
    for (int i = 0; i < m; ++i) {
        int u, v;
        infile >> u >> v >> trash; // Read each edge
        outfile << u << " " << v << " " << trash << "\n"; // Write edge to output
    }
    return costs;
}

int main(int argc, char* argv[]) {

    if(argc < 2) {
        cerr << "Usage: " << argv[0] << " <cost_type> [<min_cost> <max_cost>]" << endl;
        return 1;
    }

    string cost_type = argv[1];

    if(cost_type != "random" && cost_type != "linear" && cost_type != "logarithmic") {
        cerr << "Invalid cost type. Use 'random', 'linear', or 'logarithmic'." << endl;
        return 1;
    }

    if(cost_type == "random" && argc != 5) {
        cerr << "For random costs, provide min and max cost values." << endl;
        return 1;
    }

    string out_path = "costs.bin";
    string txt_path = "costs.txt";

    vector<double> costs;

    if(cost_type == "random") {
        double min_cost = stod(argv[2]);
        double max_cost = stod(argv[3]);
        int n = get_degrees_from_file(argv[4]).size(); // Assuming the graph file is the 4th argument
        costs = get_random_costs(n, min_cost, max_cost);

        out_path = string(argv[4]) + "_costs.bin";
        txt_path = string(argv[4]) + "_costs.txt";
    } else if (cost_type == "linear" || cost_type == "logarithmic") {
        // Example degree distribution for demonstration
        vector<int> d_out = get_degrees_from_file(argv[2]);
        if (cost_type == "linear") {
            costs = normalized_linear_costs(d_out);
        } else {
            costs = normalized_logarithmic_costs(d_out);
        }

        out_path = string(argv[2]) + "_costs.bin";
        txt_path = string(argv[2]) + "_costs.txt";
    }

    write_bincode_vector(out_path, costs);
    write_txt_costs(txt_path, costs);
    std::cout << "Wrote binary costs to: " << out_path << '\n';
    std::cout << "Wrote text costs to:   " << txt_path << '\n';

    return 0;
}
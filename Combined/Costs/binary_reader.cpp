#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <iostream>


std::vector<double> read_bincode_vector(const std::string& in_path) {
    std::ifstream in(in_path, std::ios::binary | std::ios::in);
    std::vector<double> values;
    if (!in) {
        throw std::runtime_error("Unable to open input bin file: " + in_path);
    }

    // Read the length of the vector (u64)
    uint64_t len = 0;
    for (int i = 0; i < 8; ++i) {
        char byte;
        in.get(byte);
        len |= (static_cast<uint64_t>(static_cast<uint8_t>(byte)) << (8 * i));
    }

    values.resize(len);

    // Read each double value
    for (uint64_t i = 0; i < len; ++i) {
        uint64_t bits = 0;
        for (int j = 0; j < 8; ++j) {
            char byte;
            in.get(byte);
            bits |= (static_cast<uint64_t>(static_cast<uint8_t>(byte)) << (8 * j));
        }
        std::memcpy(&values[i], &bits, sizeof(bits));
    }

    return values;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_bin_file>" << std::endl;
        return 1;
    }
    std::vector<double> costs = read_bincode_vector(argv[1]);
    std::ofstream txt_out(argv[1] + std::string(".txt"));
    for (const auto& cost : costs) {
        txt_out << cost << std::endl;
    }
    txt_out.close();

    return 0;
}
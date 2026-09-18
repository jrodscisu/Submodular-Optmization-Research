#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

using namespace std;

int main() {
    // You can modify these values or read them from standard input
    int k, l;
    if (!(cin >> k >> l)) {
        // Default values if no input is provided
        k = 3;
        l = 10; 
    }

    // Total number of vertices
    // Left side: 2k nodes (0 to 2k - 1)
    // Right side: k*l nodes (2k to 2k + k*l - 1)
    int n = 2 * k + k * l;
    
    vector<pair<int, int>> edges;

    // Phase 1: 
    // Node i (from 0 to k - 1) connects to every vertex in S_i.
    // Set S_i consists of l nodes starting at offset (2k + i * l).
    for (int i = 0; i < k; ++i) {
        for (int step = 0; step < l; ++step) {
            int right_node = 2 * k + i * l + step;
            edges.push_back({i, right_node});
        }
    }

    // Phase 2:
    // Nodes k to 2k - 1 pick "a little more than 1/k fraction" from each S_i.
    // We use the ceiling of l / k to define this fraction.
    
    // Track the index of the first available (unpicked) node in each S_i
    vector<int> available_start(k, 0);
    int curr = 0;
    for (int v = k; v < 2 * k; ++v) {
        int pick_count = (k * l - curr + (k * k - 1)) / (k * k); 
        for (int i = 0; i < k; ++i) {
            int start = available_start[i];
            
            // Ensure we don't pick more nodes than S_i actually has
            int end = min(l, start + pick_count); 
            
            for (int step = start; step < end; ++step) {
                int right_node = 2 * k + i * l + step;
                edges.push_back({v, right_node});
            }

            curr += (end - start); // Update the total number of nodes picked
            
            // Update the available pool for the next left-side vertex
            available_start[i] = end;
        }
    }

    // Randomize the edges
    random_device rd;
    mt19937 g(rd());
    shuffle(edges.begin(), edges.end(), g);

    // Output the results in the requested format
    cout << n << " " << edges.size() << "\n";
    for (const auto& edge : edges) {
        cout << edge.first << " " << edge.second << " 1\n";
    }

    return 0;
}
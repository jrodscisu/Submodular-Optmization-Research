#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <set>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <random>

#include "../LibMVC/LibMVC/fastvc.hpp"

using namespace libmvc;

std::vector<std::vector<int>> graph;
std::priority_queue<std::pair<int, int> > heap;
std::set<int> S;
std::set<int> prediction;
std::vector<int> f;

int v_num, e_num, K;

std::vector<int> read_graph(std::string filename){
    std::ifstream str(filename, std::ios::in);

    char line[1024];
    char tempstr1[10];
    char tempstr2[10];
    int e;

    char tmp;
    int v1, v2;

    str.getline(line, 1024);
    while (line[0] != 'p') str.getline(line, 1024);
    sscanf(line, "%s %s %d %d", tempstr1, tempstr2, &v_num, &e_num);

    graph.assign(v_num, std::vector<int>(std::vector<int> ()));

    std::vector<int> v_degree(v_num);
    for (e = 0; e < e_num; ++e) {
      str >> tmp >> v1 >> v2;
      // start counting at zero
      --v1;
      --v2;
      ++(v_degree[v1]);
      ++(v_degree[v2]);

      graph[v1].push_back(v2);
      graph[v2].push_back(v1);
    }

    std::string s;

    if(str.eof()){
        std::cerr << "No k provided" << std::endl;
        exit(0);
    }else{
        getline(str, s);

        str >> std::ws >> v1;

        K = v1;

        getline(str, s);
        prediction.clear();

        for(int i = 0; i < v_num; i++)
            prediction.insert(i);

        while(!str.eof()){
            str >> v1;

            if(prediction.find(v1 - 1) != prediction.end()){
                prediction.erase(prediction.find(v1 - 1));
            }
        }
    }

    while(!heap.empty())
        heap.pop();

    for(int u = 0; u < v_num; u++){
        heap.push({v_degree[u], u});
    }

    return v_degree;
}

int arg_max() {
    if(heap.empty()){
        std::cerr << "Empty heap" << std::endl;
        exit(1);
    }
    int u, val;
    do{
        u = heap.top().second;
        val = heap.top().first;

        //std::cout << u  << " " << val << " " << f[u] << std::endl;
        if(val > f[u]){
            heap.pop();
            heap.push({f[u], u});
        }
    } while(val > f[u]);

    return u;
}

void update_f(int u) {
    f[u] = 0;
    for(auto v : graph[u]){
        f[v] = std::max(0, f[v] - 1);
    }
}

void remove_from_S(int u,bool from_greedy){
    if(from_greedy){
        heap.pop();
    }

    if(prediction.find(u) != prediction.end()) {
        prediction.erase(prediction.find(u));
    }

    update_f(u);
}

std::pair<std::set<int>, int> optimal_hints_algo(int k) {
    S.clear();
    int answer = 0;    

    while(S.size() < k){
        int v = arg_max();

        S.insert(v);

        answer += f[v];
        remove_from_S(v, true);
    }

    return make_pair(S, answer);
}

std::pair<std::set<int>, int> optimal_hints_algo_w_oracle(int k){
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed); 

    int min = 1;
    int max = 100;
    std::uniform_int_distribution<int> distribution(min, max);

    S.clear();
    int answer = 0;
    int v, p;
    int coin;
    double Beta;
    bool used_v;
    while(S.size() < k){
        used_v = true;
        v = arg_max();
        if(!prediction.empty()){
            p = *(prediction.begin()); 
            Beta = (double)f[p]/(f[p] + f[v]);

            coin = distribution(generator);

            if(coin <= 100 * Beta){
                used_v = false;
                v = p;
            }
        }
        // std::cerr << heap.size() << std::endl;

        S.insert(v);

        answer += f[v];
        remove_from_S(v, used_v);
    }

    return make_pair(S, answer);
}


int main(int argc, char * argv[]) {
    std::vector<int> init_f = read_graph(argv[1]);

    f.assign(init_f.begin(), init_f.end());
    std::pair<std::set<int>, int> returned_pair = optimal_hints_algo(K); 

    std::cout << "Optimal answer: " << e_num << std::endl;
    std::cout << "The greedy algorithm found the maximum number of edges to be covered as: " << returned_pair.second << std::endl;
    std::cout << "The ratio w.r.t. the optimal: " << ((double)returned_pair.second)/e_num << std::endl;

    //Predictions

    while(!heap.empty()){
        heap.pop();
    }

    f.assign(init_f.begin(), init_f.end());

    for(int i = 0; i < v_num; i++){
        heap.push({f[i], i});
    }

    std::pair<std::set<int>, int> returned_pair_w_oracle = optimal_hints_algo_w_oracle(K); 

    std::cout << std::endl << "The OptimalHints algorithm found the maximum number of edges to be covered as: " << returned_pair_w_oracle.second << std::endl;
    std::cout << "The ratio w.r.t. the optimal: " << ((double)returned_pair_w_oracle.second)/e_num << std::endl;
    // std::cout << "The elements are:";

    // for(auto x : returned_pair.first){
    //     std::cout << " " << x;
    // }

    return 0;
}
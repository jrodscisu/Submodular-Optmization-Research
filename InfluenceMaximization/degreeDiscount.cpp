#include <bits/stdc++.h>

using namespace std;

map<set<int>, double> memory_map; 

template <typename T>
struct Edge{
    T target;
    double pr;
};

template <typename T>
class Graph{
    public:

    int size;
    vector<vector<Edge<T>>> adjList;
    vector<int> d, t;
    vector<double> dd;

    Graph(int n){
        size = n;
        d.assign(n, 0);
        t.resize(n);
        dd.resize(n);
        adjList.assign(n, vector<Edge<T>> ());

    }

    void addEdge(T u, T v, double pr){

        adjList[u].push_back({v, pr});
        // adjList[v].push_back({u, pr});
    
        d[v]++;
        // d[u]++;
    }
};


class IMM{
private:

    int num_nodes;
    vector<vector<Edge<int>>> reverse_graph;
    vector<vector<int>> RR_sets;
    vector<vector<int>> node_to_rr_forMG;
    vector<int> degrees_forMG;
    vector<bool> covered_sets_forMG;
    int curr_covered_sets_forMG;
    int theta;
    mt19937 rng;

public:

    IMM(int n) : num_nodes(n) {
        reverse_graph.resize(n);
        random_device rd;
        rng.seed(rd());
    }

    void add_edge(int u, int v, double p) {
        reverse_graph[v].push_back({u, p});
    }
    
    double log_n_choose_k(int n, int k){

        return lgamma(n + 1) - lgamma(k + 1) - lgamma(n - k + 1);
    }

    vector<int> generate_rr_set(int start_node){
        vector<int> rr_set;
        queue<int> q;
        vector<bool> visited(num_nodes, false);
        uniform_real_distribution<double> dist(0.0, 1.0);

        q.push(start_node);
        visited[start_node] = true;
        rr_set.push_back(start_node);

        while(!q.empty()){
            int u = q.front();
            q.pop();

            for(const Edge<int> & edge: reverse_graph[u]){
                int v = edge.target;

                //Modified to acknowledge node removal

                if(!visited[v]) {
                    if(dist(rng) <= edge.pr) {
                        visited[v] = true;
                        q.push(v);
                        rr_set.push_back(v);
                    }
                }
            }
        }

        return rr_set;
    }

    vector<int> node_selection(const vector<vector<int> > & rr_sets, int k){

        vector<int> seed_set;
        int num_sets = rr_sets.size();
        
        vector<vector<int> > node_to_rr(num_nodes);

        vector<int> degrees(num_nodes, 0);

        for(int i = 0;i < num_sets; i++){
            for(int node : rr_sets[i]){
                node_to_rr[node].push_back(i);
                degrees[node]++;
            }
        }

        vector<bool> covered_sets(num_sets, false);

        for(int i = 0; i < k; i++){
            int best_node = max_element(degrees.begin(), degrees.end()) - degrees.begin();
            
            if(degrees[best_node] == 0) break;

            seed_set.push_back(best_node);

            for(int set_idx : node_to_rr[best_node]){
                if(!covered_sets[set_idx]){
                    covered_sets[set_idx] = true;

                    for(int v : rr_sets[set_idx]){
                        degrees[v]--;
                    }
                }
            }
        }

        return seed_set;
    }

    int count_covered_sets(const vector<vector<int> > & rr_sets, vector<int> seeds){
        int num_sets = rr_sets.size();
        vector<bool> covered(num_sets, false);
        int covered_count = 0;

        for(int i = 0; i < num_sets; i++){
            for(int node : rr_sets[i]){
                if(find(seeds.begin(), seeds.end(), node) != seeds.end()){
                    covered[i] = true;
                    covered_count++;
                    break;
                }
            }
        }

        return covered_count;
    }

    int compute_theta(int k, double epsilon = 0.1, double l = 0.1){
        double n = num_nodes;
        double e = exp(1.0);
        thread_local uniform_int_distribution<int> node_dist(0, n - 1);

        // cout << "=== Starting parameter Estimation ===" << endl;

        //calculating the theoretical ceiling of lambda star

        double alpha = sqrt(l * log(n) + log(2.0));
        double beta = sqrt((1.0 - 1.0 / e) * (log_n_choose_k(n, k) + l * log(n) + log(2.0)));

        double lambda_star = 2.0 * n * pow(((1.0 - 1.0 / e) * alpha + beta), 2)/ pow(epsilon, 2.0);

        // Iterative scouting to find lower bound LB

        double epsilon_prime = epsilon * sqrt(2.0);
        double LB = 1.0;

        for (int i = 1; i < log2(n); i++){
            double x = n / pow(2.0, i);


            //Calculate the number or RR sets to generate for this iteration
            double theta_i = (2.0 + (2.0 / 3.0) * epsilon_prime) * (log_n_choose_k(n, k) + l * log(n) + log(log2(n))) * n / (pow(epsilon_prime, 2) * x);

            int num_rr_sets = ceil(theta_i);

            // Generate the small batch of RR sets

            vector<vector<int>> RR_sets(num_rr_sets);

            // cout << ">>>>>>> Generating rr_sets" << endl;

            for (int j = 0; j < num_rr_sets; j++){
                int random_node = node_dist(rng);
                RR_sets[j] = generate_rr_set(random_node);
            }

            // cout << ">>>>>>> Selecting nodes" << endl;

            vector<int> seeds = node_selection(RR_sets, k);

            // cout << ">>>>>>> Counting covered sets" << endl;

            int covered_count = count_covered_sets(RR_sets, seeds);
            double fraction_covered = (double) covered_count / num_rr_sets;


            //Martingale condition
            if(n * fraction_covered >= (1.0 + epsilon_prime) * x){
                LB = (n * fraction_covered) / (1.0 + epsilon_prime);
                // cout << "Treshold crossed at iteration " << i << ". Lower Bound found: " << LB << endl;
                break;
            }
        }
        
        double theta = lambda_star / LB;

        // cout << "Calculated Optimal Theta: "  << ceil(theta) << " RR Sets " << endl;

        return ceil(theta);
    }

    pair<vector<int>, double> run(int k){
        
        get_RR_sets(k);

        vector<int> final_seeds = node_selection(RR_sets, k);

        int covered = count_covered_sets(RR_sets, final_seeds);
        double estimated_spread = (double) num_nodes * ((double)covered / theta);

        return {final_seeds, estimated_spread};
    }

    void get_RR_sets(int k) {
        
        theta = compute_theta(k);
        RR_sets.resize(theta);
        uniform_int_distribution<int> node_dist(0, num_nodes - 1);

        // cout << "=== Starting Phase 2: Core execution ===" << endl;

        for(int i = 0; i < theta; i++) {
            int random_node = node_dist(rng);
            RR_sets[i] = generate_rr_set(random_node);
        }

        node_to_rr_forMG.resize(num_nodes);
        degrees_forMG.assign(num_nodes, 0);

        int num_sets = RR_sets.size();

        for(int i = 0;i < num_sets; i++){
            for(int node : RR_sets[i]){
                node_to_rr_forMG[node].push_back(i);
                degrees_forMG[node]++;
            }
        }

        covered_sets_forMG.assign(num_sets, false);

        curr_covered_sets_forMG = 0;
    }

    double compute_influence(const vector<int> & seeds){

        int covered = count_covered_sets(RR_sets, seeds);

        double estimated_spread = (double) num_nodes * ((double) covered / theta);

        return estimated_spread;
    }

    void reset_forMG() {
        node_to_rr_forMG.assign(num_nodes, vector<int>());
        degrees_forMG.assign(num_nodes, 0);

        int num_sets = RR_sets.size();

        for(int i = 0;i < num_sets; i++){
            for(int node : RR_sets[i]){
                node_to_rr_forMG[node].push_back(i);
                degrees_forMG[node]++;
            }
        }

        covered_sets_forMG.assign(num_sets, false);

        curr_covered_sets_forMG = 0;
    }

    int getMG_for_node(int p){
        return degrees_forMG[p];
    }

    pair<int, double>  node_selection_next_wo_S(){

        int be = max_element(degrees_forMG.begin(), degrees_forMG.end()) - degrees_forMG.begin();
        return {be, degrees_forMG[be]};
    }

    vector<pair<int, double>>  r_predictions(int r){
        priority_queue<pair<int, int> , vector<pair<int, int> > , greater<pair<int, int>> > pq;
        vector<pair<int, double>> ps;

        for(int i = 0; i < r; i++){
            pq.push({degrees_forMG[i], i});
        }

        for(int i = r; i < num_nodes; i++){
            if(degrees_forMG[i] > pq.top().second){
                pq.pop();
                pq.push({degrees_forMG[i], i});
            }
        }

        while(!pq.empty()){
            auto [du, u] = pq.top();
            ps.push_back({u, du});
            pq.pop();
        }

        return ps;
    } 

    void update_node_selection(int u) {
        
        assert(u < (int)node_to_rr_forMG.size());
        for(int set_idx : node_to_rr_forMG[u]){
            if(!covered_sets_forMG[set_idx]){
                covered_sets_forMG[set_idx] = true;
                curr_covered_sets_forMG++;

                assert(set_idx < (int)RR_sets.size());
                for(int v : RR_sets[set_idx]){
                    degrees_forMG[v]--;
                }
            }
        }
    }

    double estimated_spread(double covered){
        return (double) num_nodes * ((double)(covered + curr_covered_sets_forMG) / theta);
    }
};



vector<int> degreeDiscountIC(Graph<int> & G, int k, int p){
    set<int> S;
    priority_queue<pair<double, int> > VmS;

    for(int i = 0; i < G.size; i++){

        G.dd[i] = G.d[i];
        G.t[i] = 0;
        VmS.push({G.dd[i], i});
    }

    for(int i = 0; i < k; i++){

        auto [val, u] = VmS.top(); VmS.pop();

        while(S.find(u) != S.end() || G.dd[u] != val){
            val = VmS.top().first; 
            u = VmS.top().second;
            VmS.pop();
        }

        S.insert(u);

        for(auto [v, pr] : G.adjList[u]){
            G.t[v]++;
            G.dd[v] = (double)G.d[v] - 2.0 * G.t[v] - (double)(G.d[v] - G.t[v]) * G.t[v] * ((double)p/100);

            VmS.push({G.dd[v], v});
        }
    }

    vector<int> Sv(S.begin(), S.end());

    return Sv;
}

int IC(const Graph<int> & G, const vector<int> & S){

    thread_local std::random_device rd;  
    thread_local std::mt19937 gen(rd()); 
    thread_local std::uniform_real_distribution<double> coin(0.0, 1.0); 

    vector<uint8_t> influenced(G.size, 0);

    queue<int> active;

    for(auto x: S){
        influenced[x] = 1;
        active.push(x);
    }

    int ans = 0;

    while(!active.empty()){
        ans++;
        int u = active.front(); active.pop();

        for(auto [v, pr]: G.adjList[u]){
            if(influenced[v])
                continue;

            if(coin(gen) <= pr){
                influenced[v] = 1;
                active.push(v);
            }
        }
    }

    return ans;
}

double monte_carlo_IC(vector<int> & seeds, Graph<int> &G, int reps){
    double sum = 0.0;

    #pragma omp parallel for reduction(+:sum)
    for(int i = 0; i < reps; i++){
        sum += IC(G, seeds);
    }

    return sum / reps;
}

Graph<int> readGraph(string filename){
    ifstream file;
    file.open(filename);

    int n, m;
    double p;

    file >> n >> m;

    map<int, int> compress;

    Graph<int> G(n);

    int u, v;

    for(int i = 0; i < m; i++){
        file >> u >> v >> p;

        if(compress.find(u) == compress.end()){
            compress.insert({u, compress.size()});
        }

        if(compress.find(v) == compress.end()){
            compress.insert({v, compress.size()});
        }

        // G.addEdge(compress[u], compress[v], p); 

        // Zero-idx solution

        G.addEdge(u, v, p); 
    }

    file.close();

    return G;
}

vector<vector<int>> readOptimal(string filename, int n){
    ifstream fin;
    fin.open(filename);

    vector<vector<int>> v;

    int x;

    for(int i = 1; i <= n; i++){
        v.push_back(vector<int>());
        for(int j = 0; j < i; j++){
            fin >> x;
            v.back().push_back(x);
        }
    }

    fin.close();

    return v;
}

pair<set<int> , double> OH_OPT_k(Graph<int> & G, int k, double pr, int MC_reps, IMM & im){

    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    //Init graph in IMM


    vector<double> g_seed;

    priority_queue<pair<double, int> > VmS;

    set<int> S;
    vector<int> S_prime;

    // Setting up IMM

    im.reset_forMG();

    for(int i = 0; i < G.size; i++){

        G.dd[i] = G.d[i];
        G.t[i] = 0;
        VmS.push({G.dd[i], i});
    }

    double mp, prev = 0.0;

    for(int i = 0; i < k; i++){

        // cout << "Getting dd candidate " << i << endl;
        auto [val, p] = VmS.top();

        while(S.find(p) != S.end() || G.dd[p] != val){
            VmS.pop();
            val = VmS.top().first; 
            p = VmS.top().second;
        }

        // cout << "Getting Greedy candidate " << i << endl;

        auto [g, mg] = im.node_selection_next_wo_S();

        // mg = im.estimated_spread(mg);
        S_prime.push_back(p);
        set<int> temp(S_prime.begin(), S_prime.end());
        if(memory_map.find(temp) == memory_map.end()) {
            mp = monte_carlo_IC(S_prime, G, MC_reps);
            memory_map.insert({temp, mp});
        }else {
            mp = memory_map[temp];
        }
        S_prime.pop_back();

        if(g != p) {
            S_prime.push_back(g);
            set<int> temp2(S_prime.begin(), S_prime.end());
            if(memory_map.find(temp2) == memory_map.end()) {
                mg = monte_carlo_IC(S_prime, G, MC_reps);
                memory_map.insert({temp2, mg});
            }else{
                mg = memory_map[temp2];
            }
            S_prime.pop_back();
        }else {
            mg = mp;
        }


        int v = g;

        mp -= prev;
        mg -= prev;

        if(coin(gen) <= (mp / (mp + mg))){
            v = p;
            mg = mp;
            VmS.pop();
        }

        prev += mg;


        S.insert(v);
        S_prime.push_back(v);

        if((int)S.size() == k)
            break;

        // cout << "######For k = " << k << " Selected: " << v << "out of p: " << p << " and g: " << g << endl; 

        //Update greedyDiscount

        for(auto [u, pr] : G.adjList[v]){
            G.t[u]++;
            G.dd[u] = (double)G.d[u] - 2.0 * G.t[u] - (double)(G.d[u] - G.t[u]) * G.t[u] * p;

            VmS.push({G.dd[u], u});
        }

        //Update im

        im.update_node_selection(v);
    }

    // cout << ">>>>>>Done with k = " << k << endl;

    return {S, prev};
}

pair<set<int> , double> OH_iter(Graph<int> & G, int k, double pr, int MC_reps, IMM & im){

    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    //Init graph in IMM


    vector<double> g_seed;

    set<int> S;
    vector<int> S_prime;

    // Setting up IMM

    im.reset_forMG();

    for(int i = 0; i < G.size; i++){

        G.dd[i] = G.d[i];
        G.t[i] = 0;
    }

    double mp, prev = 0.0;

    int p;

    for(int i = 0; i < k; i++){

        cout << "Enter prediction: " << endl;
        cerr << "Enter prediction: ";
        cin >> p;
        cerr << p << endl;
        
        auto [g, mg] = im.node_selection_next_wo_S();

        // mg = im.estimated_spread(mg);
        S_prime.push_back(p);
        set<int> temp(S_prime.begin(), S_prime.end());
        if(memory_map.find(temp) == memory_map.end()) {
            mp = monte_carlo_IC(S_prime, G, MC_reps);
            memory_map.insert({temp, mp});
        }else {
            mp = memory_map[temp];
        }
        S_prime.pop_back();

        if(g != p) {
            S_prime.push_back(g);
            set<int> temp2(S_prime.begin(), S_prime.end());
            if(memory_map.find(temp2) == memory_map.end()) {
                mg = monte_carlo_IC(S_prime, G, MC_reps);
                memory_map.insert({temp2, mg});
            }else{
                mg = memory_map[temp2];
            }
            S_prime.pop_back();
        }else {
            mg = mp;
        }


        int v = g;

        mp -= prev;
        mg -= prev;

        if(coin(gen) <= (mp / (mp + mg))){
            v = p;
            mg = mp;
        }

        prev += mg;


        S.insert(v);
        S_prime.push_back(v);

        cerr << "######For k = " << k << " Selected: " << v << " out of p: " << p << " and g: " << g << endl; 
        cout << "######For k = " << k << " Selected: " << v << " out of p: " << p << " and g: " << g << endl; 
        cout << "S = {" ;
        cerr << "S = {" ;
        
        for(auto x : S){
            cout << x << ", ";
            cerr << x << ", ";
        }
        cout << endl;
        cerr << endl;


        //Update greedyDiscount

        for(auto [u, pr] : G.adjList[v]){
            G.t[u]++;
            G.dd[u] = (double)G.d[u] - 2.0 * G.t[u] - (double)(G.d[u] - G.t[u]) * G.t[u] * p;

        }

        //Update im

        im.update_node_selection(v);
    }

    // cout << ">>>>>>Done with k = " << k << endl;

    return {S, prev};
}

pair<set<int> , double> OH_fixedPred_old(Graph<int> & G, int k, double pr, int MC_reps, IMM & im, vector<int> ps){

    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    //Init graph in IMM

    // cout << "Running OH with fixed prediction..." << endl;

    vector<double> g_seed;

    set<int> S;
    vector<int> S_prime;

    // Setting up IMM

    im.reset_forMG();

    
    double mp, prev = 0.0;

    int p;

    for(int i = 0; i < k; i++){

        p = ps[0];
        
        auto [g, mg] = im.node_selection_next_wo_S();

        S_prime.push_back(p);
        set<int> temp(S_prime.begin(), S_prime.end());
        mp = im.compute_influence(S_prime);
        S_prime.pop_back();

        if(g != p) {
            S_prime.push_back(g);
            set<int> temp2(S_prime.begin(), S_prime.end());
            mg = im.compute_influence(S_prime);
            S_prime.pop_back();
        }else {
            mg = mp;
        }


        int v = g;

        mp -= prev;
        mg -= prev;

        if(coin(gen) <= (mp / (mp + mg))){
            v = p;
            mg = mp;
        }

        prev += mg;


        S.insert(v);
        S_prime.push_back(v);


        int x = find(ps.begin(), ps.end(), v) - ps.begin();

        if(x < (int)ps.size()){
            ps.erase(ps.begin() + x);
        }

        //Update im

        im.update_node_selection(v);
    }

    return {S, prev};
}

pair<set<int> , double> OH_fixedPred(Graph<int> & G, int k, double pr, int MC_reps, IMM & im, vector<int> ps){

    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    //Init graph in IMM

    // cout << "Running OH with fixed prediction..." << endl;

    vector<double> g_seed;

    set<int> S;
    vector<int> S_prime;

    // Setting up IMM

    im.reset_forMG();

    
    double mp, prev = 0.0;

    int p;

    for(int i = 0; i < k; i++){

        p = ps[0];
        
        auto [g, mg] = im.node_selection_next_wo_S();

        S_prime.push_back(p);
        set<int> temp(S_prime.begin(), S_prime.end());
        // mp = im.compute_influence(S_prime);
        mp = im.getMG_for_node(p);
        S_prime.pop_back();

        // if(g != p) {
        //     S_prime.push_back(g);
        //     set<int> temp2(S_prime.begin(), S_prime.end());
        //     mg = im.compute_influence(S_prime);
        //     S_prime.pop_back();
        // }else {
        //     mg = mp;
        // }


        int v = g;

        if(coin(gen) <= (mp / (mp + mg))){
            v = p;
            mg = mp;
        }

        prev += mg;


        S.insert(v);
        S_prime.push_back(v);


        int x = find(ps.begin(), ps.end(), v) - ps.begin();

        if(x < (int)ps.size()){
            ps.erase(ps.begin() + x);
        }

        //Update im

        im.update_node_selection(v);
    }

    return {S, im.estimated_spread(0)};
}

pair<set<int> , double> OH_topr(Graph<int> & G, int k, double pr, int MC_reps, IMM & im, int r){

    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> coin(0.0, 1.0);
    std::uniform_int_distribution p_node(0, r - 1);
    //Init graph in IMM

    // cout << "Running OH with fixed prediction..." << endl;

    vector<double> g_seed;

    set<int> S;
    vector<int> S_prime;

    // Setting up IMM

    im.reset_forMG();

    
    double mp, prev = 0.0;

    int p;

    for(int i = 0; i < k; i++){

        vector<pair<int, double>>  predictions = im.r_predictions(r); 
        int random_pred = p_node(gen);
        p = predictions[random_pred].first;
        
        auto [g, mg] = im.node_selection_next_wo_S();

        S_prime.push_back(p);
        set<int> temp(S_prime.begin(), S_prime.end());
        // mp = im.compute_influence(S_prime);
        mp = im.getMG_for_node(p);
        S_prime.pop_back();

        // if(g != p) {
        //     S_prime.push_back(g);
        //     set<int> temp2(S_prime.begin(), S_prime.end());
        //     mg = im.compute_influence(S_prime);
        //     S_prime.pop_back();
        // }else {
        //     mg = mp;
        // }


        int v = g;

        if(coin(gen) <= (mp / (mp + mg))){
            v = p;
            mg = mp;
        }

        prev += mg;


        S.insert(v);
        S_prime.push_back(v);

        //Update im

        im.update_node_selection(v);
    }

    return {S, im.estimated_spread(0)};
}

vector<int> singleDiscount(Graph<int> & G, int k){
    set<int> S;
    priority_queue<pair<double, int> > VmS;

    for(int i = 0; i < G.size; i++){

        G.dd[i] = G.d[i];
        VmS.push({G.dd[i], i});
    }

    for(int i = 0; i < k; i++){


        auto [val, g] = VmS.top(); VmS.pop();

        while(S.find(g) != S.end() || G.dd[g] != val){
            val = VmS.top().first; 
            g = VmS.top().second;
            VmS.pop();
        }

        S.insert(g);

        for(auto [u, pr] : G.adjList[g]){
            G.dd[u]--;
            VmS.push({G.dd[u], u});
        }
    }

    vector<int> Sv(S.begin(), S.end());

    return Sv;
}

int main(int argc, char * argv[]) {

    ofstream log_output("logs.txt", std::ios::app);

    auto old_rdbuf = clog.rdbuf();

    clog.rdbuf(log_output.rdbuf());

    int k = 1;
    string graph_file = "data/CA-GrQc.txt", optimal_file;
    vector<vector<int>> opt;

    if(argc > 1){
        k = atoi(argv[1]);
        graph_file = argv[2];
        optimal_file = argv[3];
        opt = readOptimal(optimal_file, k);
    }

    Graph<int> G = readGraph("data/" + graph_file + ".txt");

    IMM im(G.size);
    IMM im_oh(G.size);


    for(int i = 0; i < G.size; i++){
        for(auto [u, pr] : G.adjList[i]){
            im.add_edge(i, u, pr);
            im_oh.add_edge(i, u, pr);
        }
    }

    for(int z = 0; z < k; z++){

        clog << "######################################################" << endl;
        clog << "             Starting OH with K = " << z + 1 << endl;
        clog << "            " << __DATE__ << ", Graph: " << graph_file <<  endl;
        clog << "######################################################" << endl;
        cout << "######################################################" << endl;
        cout << "             Starting OH with K = " << z + 1 << endl;
        cout << "######################################################" << endl;


        auto [output, greedy_spread] = im.run(z + 1);
        im_oh.run(z + 1);

        double value;

        greedy_spread = im_oh.compute_influence(output);

        ofstream fout;

        clog << "OPT: " << z;
        cout << "OPT: ";
        for(auto x : opt[z]) {
            cout << x << ' ';
            clog << x << ' ';
        }

        value = im_oh.compute_influence(opt[z]);
        clog << endl << "Total influence spread: " << value << endl;
        cout << endl << "Total influence spread: " << value << endl;

        cout << "Done with OPT" << endl;

        fout.open("results/"+graph_file+"_OPT.txt", std::ios::app);
        fout << "(" << z + 1 << ", " << value << ")" << endl;
        fout.close();
        
        clog << "Greedy: ";
        cout << "Greedy: ";
        for(auto x : output){
        clog << x << ' ';
        cout << x << ' ';
        }
        
        clog << endl << "Total influence spread: " << greedy_spread << endl;
        cout << endl << "Total influence spread: " << greedy_spread << endl;

        cout << "Done with greedy" << endl;
        
        fout.open("results/"+graph_file+"_Greedy.txt", std::ios::app);
        fout << "(" << z + 1 << ", " << value << ")" << endl;
        fout.close();

        vector<double> oh_outputs;
        set<int> diff_outputs;
        for(int i = 0; i < 50; i++){
            pair<set<int>, double> temp = OH_fixedPred(G, z + 1, 0.01, 20000, im_oh, opt[z]);

            diff_outputs.merge(temp.first);
            oh_outputs.push_back(temp.second);
        }

        sort(oh_outputs.begin(), oh_outputs.end());

        value = oh_outputs[25];

        clog << "Optimal Hints Spread: " << value << endl; 
        cout << "Optimal Hints Spread: " << value << endl; 
        cout << "Total #of distinct vertices selected as seed: " << diff_outputs.size() << endl;

        cout << "Done with OH" << endl;

        fout.open("results/"+graph_file+"_OH_alpha1.txt", std::ios::app);
        fout << "(" << z + 1 << ", " << value << ")" << endl;
        fout.close();
    }

    clog.rdbuf(old_rdbuf);

    return 0;
}
#include <bits/stdc++.h>

using namespace std;

map<set<int>, double> memory_map; 

template <typename T>
class Graph{
    public:

    int size;
    vector<vector<T>> adjList;
    vector<int> d, t;
    vector<double> dd;

    Graph(int n){
        size = n;
        d.assign(n, 0);
        t.resize(n);
        dd.resize(n);
        adjList.assign(n, vector<T> ());

    }

    void addEdge(T u, T v){

        adjList[u].push_back(v);
        adjList[v].push_back(u);
    
        d[u]++;
        d[v]++;
    }
};

struct Edge{
    int target;
    double probability;
};

class IMM{
private:

    int num_nodes;
    vector<vector<Edge>> reverse_graph;
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

            for(const Edge & edge: reverse_graph[u]){
                int v = edge.target;

                //Modified to acknowledge node removal

                if(!visited[v]) {
                    if(dist(rng) <= edge.probability) {
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

        // cout << "Estiamted Influence Spread: " << estimated_spread << " nodes " << endl;

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

    void reset_forMG() {
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

    pair<int, double>  node_selection_next_wo_S(){

        int be = max_element(degrees_forMG.begin(), degrees_forMG.end()) - degrees_forMG.begin();
        return {be, degrees_forMG[be]};
    }

    void update_node_selection(int u) {
        for(int set_idx : node_to_rr_forMG[u]){
            if(!covered_sets_forMG[set_idx]){
                covered_sets_forMG[set_idx] = true;
                curr_covered_sets_forMG++;

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



set<int> degreeDiscountIC(Graph<int> & G, int k, int p){
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

        for(auto v : G.adjList[u]){
            G.t[v]++;
            G.dd[v] = (double)G.d[v] - 2.0 * G.t[v] - (double)(G.d[v] - G.t[v]) * G.t[v] * ((double)p/100);

            VmS.push({G.dd[v], v});
        }
    }

    return S;
}

int IC(const Graph<int> & G, const vector<int> & S, double p){

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

        for(auto v: G.adjList[u]){
            if(influenced[v])
                continue;

            if(coin(gen) <= p){
                influenced[v] = 1;
                active.push(v);
            }
        }
    }

    return ans;
}

int IC(const Graph<int> & G, const set<int> & S, int p){

    thread_local std::random_device rd;  
    thread_local std::mt19937 gen(rd()); 
    thread_local std::uniform_int_distribution coin(1, 100); 

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

        for(auto v: G.adjList[u]){
            if(influenced[v])
                continue;

            if(coin(gen) <= p){
                influenced[v] = 1;
                active.push(v);
            }
        }
    }

    return ans;
}

double monte_carlo_IC(vector<int> & seeds, Graph<int> &G, double p, int reps){
    double sum = 0.0;

    #pragma omp parallel for reduction(+:sum)
    for(int i = 0; i < reps; i++){
        sum += IC(G, seeds, p);
    }

    return sum / reps;
}

Graph<int> readGraph(string filename){
    ifstream file;
    file.open(filename);

    int n, m;

    file >> n >> m;

    map<int, int> compress;

    Graph<int> G(n);

    int u, v;

    for(int i = 0; i < m; i++){
        file >> u >> v;

        if(compress.find(u) == compress.end()){
            compress.insert({u, compress.size()});
        }

        if(compress.find(v) == compress.end()){
            compress.insert({v, compress.size()});
        }

        G.addEdge(compress[u], compress[v]); 
    }

    file.close();

    return G;
}

set<int> OptimalHints_fakeGreedy(Graph<int> & G, int k, set<int> OPT){


    std::random_device rd;  
    std::mt19937 gen(rd());
    std::uniform_int_distribution coin(0, 100);

    set<int>::iterator it;

    set<int> S;
    priority_queue<pair<double, int> > VmS;

    for(int i = 0; i < G.size; i++){

        G.dd[i] = G.d[i];
        VmS.push({G.dd[i], i});
    }

    for(int i = 0; i < k; i++){

        std::uniform_int_distribution<> index(0, OPT.size() - 1);

        auto [val, g] = VmS.top(); VmS.pop();

        while(S.find(g) != S.end() || G.dd[g] != val){
            val = VmS.top().first; 
            g = VmS.top().second;
            VmS.pop();
        }

        int o = g;

        while(!OPT.empty() && o == g){
            it = OPT.begin();
            int r_steps = index(gen);

            while(r_steps--)
                it++;
            
            if(OPT.size() == 1)
                break;
            
            o = *it;
        }
        int v = (coin(gen) * (G.dd[o] + G.dd[g]) < 100 * (G.dd[o])) ? o : g;

        S.insert(v);

        for(auto u : G.adjList[v]){
            G.dd[u]--;
            VmS.push({G.dd[u], u});
        }
    }

    return S;
}

pair<set<int> , double> OptimalHints_OPT_k(Graph<int> & G, int k, double pr, int MC_reps, IMM & im){

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
            mp = monte_carlo_IC(S_prime, G, pr, MC_reps);
            memory_map.insert({temp, mp});
        }else {
            mp = memory_map[temp];
        }
        S_prime.pop_back();

        if(g != p) {
            S_prime.push_back(g);
            set<int> temp2(S_prime.begin(), S_prime.end());
            if(memory_map.find(temp2) == memory_map.end()) {
                mg = monte_carlo_IC(S_prime, G, pr, MC_reps);
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

        for(auto u : G.adjList[v]){
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

pair<set<int> , double> OptimalHints_iterative(Graph<int> & G, int k, double pr, int MC_reps, IMM & im){

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
            mp = monte_carlo_IC(S_prime, G, pr, MC_reps);
            memory_map.insert({temp, mp});
        }else {
            mp = memory_map[temp];
        }
        S_prime.pop_back();

        if(g != p) {
            S_prime.push_back(g);
            set<int> temp2(S_prime.begin(), S_prime.end());
            if(memory_map.find(temp2) == memory_map.end()) {
                mg = monte_carlo_IC(S_prime, G, pr, MC_reps);
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

        for(auto u : G.adjList[v]){
            G.t[u]++;
            G.dd[u] = (double)G.d[u] - 2.0 * G.t[u] - (double)(G.d[u] - G.t[u]) * G.t[u] * p;

        }

        //Update im

        im.update_node_selection(v);
    }

    // cout << ">>>>>>Done with k = " << k << endl;

    return {S, prev};
}
pair<set<int> , double> OptimalHints_fixedPredictions(Graph<int> & G, int k, double pr, int MC_reps, IMM & im, vector<int> ps){

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

        p = ps[0];
        
        auto [g, mg] = im.node_selection_next_wo_S();

        // mg = im.estimated_spread(mg);
        S_prime.push_back(p);
        set<int> temp(S_prime.begin(), S_prime.end());
        if(memory_map.find(temp) == memory_map.end()) {
            mp = monte_carlo_IC(S_prime, G, pr, MC_reps);
            memory_map.insert({temp, mp});
        }else {
            mp = memory_map[temp];
        }
        S_prime.pop_back();

        if(g != p) {
            S_prime.push_back(g);
            set<int> temp2(S_prime.begin(), S_prime.end());
            if(memory_map.find(temp2) == memory_map.end()) {
                mg = monte_carlo_IC(S_prime, G, pr, MC_reps);
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


        //Update greedyDiscount

        for(auto u : G.adjList[v]){
            G.t[u]++;
            G.dd[u] = (double)G.d[u] - 2.0 * G.t[u] - (double)(G.d[u] - G.t[u]) * G.t[u] * p;
        }

        int x = find(ps.begin(), ps.end(), v) - ps.begin();

        if(x < (int)ps.size()){
            ps.erase(ps.begin() + x);
        }

        //Update im

        im.update_node_selection(v);
    }

    // cout << ">>>>>>Done with k = " << k << endl;

    return {S, prev};
}

set<int> singleDiscount(Graph<int> & G, int k){
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

        for(auto u : G.adjList[g]){
            G.dd[u]--;
            VmS.push({G.dd[u], u});
        }
    }

    return S;
}

vector<pair<double,pair<double, double>>> experiment(Graph<int> & G, int maxK, int roundsSeed, int roundsIC, int p){

    double sum_greedy = 0, sum_opt = 0, sum_opt_hints = 0;
    double mean_greedy = 0, mean_opt = 0, mean_opt_hints = 0;

    vector<pair<double, pair<double, double>>> means;

    for(int k = 1; k <= maxK; k++){

        set<int> deegre_Discount = degreeDiscountIC(G, k, p);
        set<int> single_Discount = singleDiscount(G, k);

        mean_greedy = mean_opt = mean_opt_hints = 0;

        for(int i = 0; i < roundsSeed; i++){

            cout << "Round: " << k << "." << i << endl;

            set<int> S = OptimalHints_fakeGreedy(G, k, deegre_Discount);

            sum_greedy = sum_opt = sum_opt_hints = 0;

            #pragma omp parallel for reduction(+:sum_opt_hints)
            for(int j = 0; j < roundsIC; j++){

                sum_opt_hints += IC(G, S, p);
            }

            mean_opt_hints += sum_opt_hints/(roundsSeed * roundsIC);
        }

        #pragma omp parallel for reduction(+:sum_greedy, sum_opt)
        for(int i = 0; i < 20000; i++){
            sum_greedy += IC(G, single_Discount, p);
            sum_opt += IC(G, deegre_Discount, p);
        }
        
        mean_greedy += sum_greedy/(roundsIC);
        mean_opt += sum_opt/(roundsIC);

        means.push_back({mean_opt, {mean_greedy, mean_opt_hints}});
    }

    return means;
}


pair<vector<pair<vector<int>, double>>, pair< vector<pair<vector<int>, double>>, vector<pair<vector<int>, double>> > > 
    experiment_OPTk_G_DD_OH(Graph<int> &G, int maxK, double p, int MC_reps){

    vector<pair<vector<int>, double>> im_results;
    vector<pair<vector<int>, double>> dd_results;
    vector<pair<vector<int>, double>> oh_results;

    // Compute greedy using IMM for k = 1 ... maxK

    cout << "Starting with IMM: " << endl;

    IMM im(G.size);

    for(int i = 0; i < G.size; i++){
        for(int u : G.adjList[i]){
            im.add_edge(i, u, p);
        }
    }

    im.get_RR_sets(maxK);

    for(int i = 1; i <= maxK; i++){
        cout << "done with: " << endl;
        im_results.push_back({vector<int>(), 0.0});
        im.reset_forMG();
        for(int k = 1; k <= i; k++){
            im_results.back().first.push_back(im.node_selection_next_wo_S().first); 
            im.update_node_selection(im_results.back().first.back());
        }

        set<int> temp(im_results.back().first.begin(), im_results.back().first.end());
        if(memory_map.find(temp) == memory_map.end()) {
            im_results.back().second = monte_carlo_IC(im_results.back().first, G, p, MC_reps);
            memory_map.insert({temp, im_results.back().second});
        }else {
            im_results.back().second = memory_map[temp];
        }
    }

    // Compute Degree Discount

    cout << "Starting with Degree Discount" << endl;

    set<int> seed_set;

    // #pragma omp parallel for
    for(int i = 1; i <= maxK; i++){
        cout << "Working on k = " << i << endl;
        seed_set = degreeDiscountIC(G, i, p);
        dd_results.push_back({vector<int> (seed_set.begin(), seed_set.end()), 0.0});

        set<int> temp(dd_results.back().first.begin(), dd_results.back().first.end());
        if(memory_map.find(temp) == memory_map.end()) {
            dd_results.back().second = monte_carlo_IC(dd_results.back().first, G, p, MC_reps);
            memory_map.insert({temp, dd_results.back().second});
        }else {
            dd_results.back().second = memory_map[temp];
        }
    }

    //Compute OPT_k with Optimal Hints

    cout << "Starting witih OH" << endl;

    double oh_sum;

    pair<set<int>, double> temp;
    // #pragma omp parallel for
    for(int i = 1; i <= maxK; i++){
         cout << "Working on k = " << i << endl;
        oh_sum = 0.0;
        
        for(int j = 0; j < 20; j++){
            temp = OptimalHints_OPT_k(G, i, p, MC_reps, im);
            oh_sum += temp.second;
        }
        oh_results.push_back({vector<int>(), oh_sum/20});
    }

    return {dd_results, {im_results, oh_results}}; 
}

int main() {
    string files[] = {/*"GrQc", "AstroPh" , "HepTh",*/ "HepPh"};

    Graph<int> G = readGraph("data/CA-GrQc-0-idx.txt");


    vector<int> in({9, 103, 105, 281, 297, 579, 1039, 1286, 1291, 3139});
    vector<int> gem({{297, 1286, 579, 1039, 4035, 3139, 110, 188, 223, 55}});

    IMM im(G.size);

    for(int i = 0; i < G.size; i++){
        for(int u : G.adjList[i]){
            im.add_edge(i, u, 0.01);
        }
    }

    im.get_RR_sets(10);

    double sum = 0.0;

    vector<int> g;

    for(int i = 1; i <= 10; i++) {
        g.push_back(gem[i]);
        cout << "(" << i << ", " << monte_carlo_IC(g, G, 0.01, 20000)  << ")" << ' ';
    }

    cout << endl;

    return 0;



    // vector<pair<vector<int>, double>> oh_results;

    // // Compute greedy using IMM for k = 1 ... maxK

    // cout << "Starting with IMM: " << endl;

    // IMM im(G.size);

    // for(int i = 0; i < G.size; i++){
    //     for(int u : G.adjList[i]){
    //         im.add_edge(i, u, 0.01);
    //     }
    // }

    // im.get_RR_sets(10);


    // double oh_sum = 0.0;

    // pair<set<int>, double> temp;
    // for(int j = 0; j < 10; j++){
    //     temp = OptimalHints_iterative(G, 10, 0.01, 20000, im);
    //     cout << "Result: " << temp.second << endl << "Seeds selected: " << endl;

    //     for(int x : temp.first){
    //         cout << x << ' ';
    //         cerr << x << ' ';
    //     }

    //     cout << endl;
    //     cerr << endl;

    //     cerr << "Result: " << temp.second << endl;
    //     oh_sum += temp.second;
    // }

    // cout << oh_sum/10 << endl;
    // cerr << oh_sum/10 << endl;

    // gs.push_back({130, 128, 127, 131, 132});
    // gs.push_back({130, 128, 127, 131, 132, 134, 125, 126, 133, 135});
    // gs.push_back({130, 128, 127, 131, 132, 134, 125, 126, 133, 135, 124, 137, 140, 136, 123, 138, 141, 129, 298, 483});
    // gs.push_back({130, 128, 127, 131, 132, 134, 125, 126, 133, 135, 124, 137, 140, 136, 123, 138, 141, 129, 298, 483, 456, 478, 290, 355, 369, 463, 469, 496, 271, 283, 366, 388, 393, 454, 480});
    // gs.push_back({130, 128, 127, 131, 132, 134, 125, 126, 133, 135, 124, 137, 140, 136, 123, 138, 141, 129, 298, 483, 456, 478, 290, 355, 369, 463, 469, 496, 271, 283, 366, 388, 393, 454, 480, 253, 263, 272, 281, 292, 299, 356, 361, 376, 381, 396, 459, 471, 475, 493});

//     vector<int> gs({126, 356, 19,355,133,483,127,296,130,263,125,472,132,283,27,457,136,262,135,498,140,467,123,377,381,393,124,366,128,137,480,290,131,373,280,469,259,298,379,496,292,134,99,493,475,256,297,455,359,294});
    
//     vector<int> gpt({
// 356, 355, 483, 469, 480, 475, 490, 492, 496, 499,
// 500, 487, 471, 472, 474, 476, 478, 481, 482, 485,
// 463, 462, 464, 465, 467, 468, 470, 473, 477, 479,
// 451, 452, 453, 454, 455, 456, 457, 458, 459, 460,
// 350, 351, 352, 353, 354, 357, 359, 361, 17, 27
// });

//     vector<int> claude({{17, 19, 27,
//  123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
//  133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
//  250, 251, 252, 253, 254, 255, 256, 257, 258,
//  350, 351, 352, 353, 354, 355, 356, 357, 358,
//  450, 451, 452, 453, 454, 455, 456, 457, 458}});

//     vector<int> g;
//     for(int i = 0; i < gpt.size(); i++){
//         g.push_back(gpt[i] - 1);
//         cout << "(" << i + 1 << ", " << monte_carlo_IC(g, G, 0.01, 20000) << ") \n";
//     }

//     cout << endl;


    for(int j = 0; j < 1; j++){

        // Graph<int> G = readGraph("data/girth11.txt");


        pair<vector<pair<vector<int>, double>>, pair< vector<pair<vector<int>, double>>, vector<pair<vector<int>, double>> > > results = experiment_OPTk_G_DD_OH(G, 15, 0.01, 20000);

        cout << "##################################\nResults for " + files[j] << endl;
        cout << "Degree discount: ";
        for(int i = 0; i < (int)results.first.size(); i++){
            cout << results.first[i].second << ' ';
        }
        cout << endl;

        cout << "Greedy: ";
        for(int i = 0; i < (int)results.second.first.size(); i++){
            cout << results.second.first[i].second << ' ';
        }
        cout << endl;

        cout << "Optimal Hints: ";
        for(int i = 0; i < (int)results.second.second.size(); i++){
            cout << results.second.second[i].second << ' ';
        }
        cout << endl;
    }

    cout << "Gemini: (10, " << monte_carlo_IC(in, G, 0.01, 20000) << ")" << endl;

    /*IMM Only*/

    // IMM im(G.size);

    // for(int i = 0; i < G.size; i++){
    //     for(int u : G.adjList[i]){
    //         im.add_edge(i, u, 0.01);
    //     }
    // }

    // vector<double> results;

    // for(int i = 1; i < 51; i++){
    //     results.push_back(im.run(i).second);
    // }

    // cout << "IMM results:" << endl;
    // for(auto x : results){
    //     cout << x << ' ';
    // }

    // cout << endl;

    /*First set of experiments*/

    // //vector<pair<double, pair<double, double>>> G_25_results = experiment(G, 50, 30, 20000, 1);

    // cout << "OPT: \t";

    // for(auto x : G_25_results){
    //     cout << x.first << ' ';
    // }

    // cout << endl << "Greedy: \t";

    // for(auto x : G_25_results){
    //     cout << x.second.first << ' ';
    // }

    // cout << endl << "Optimal Hints: \t";

    // for(auto x : G_25_results){
    //     cout << x.second.second << ' ';
    // }
    
    // cout << endl;


    return 0;
}
#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <sstream>
#include <fstream>

using namespace std;

class MovieRecommendation{
    public:
        int user_n, movie_n;
        vector<int> covered;
        vector<double> cost_movie;
        vector<int> user_per_movie;
        vector<vector<int>> user_list;
        vector<vector<int>> movie_list;

        MovieRecommendation(int user, int movie){
            user_n = user;
            movie_n = movie;
            covered.assign(user_n, 0);
            cost_movie.assign(movie_n, 1);
            user_per_movie.assign(movie_n, 1);
            user_list.assign(movie_n, vector<int>());
            movie_list.assign(user_n, vector<int>());
        }

        pair<int, vector<int>> compute_coverage_SG(int k){
            int n = movie_n;
            int coverage = 0;

            priority_queue<pair<int, int> > pq;

            vector<int> user_per_movie_temp = user_per_movie;
            vector<int> user_covered(user_n, 0);

            for(int i = 0; i < n; i++){
                pq.push({user_per_movie_temp[i], i});
            }

            vector<int> seeds;

            while((int)seeds.size() < k && !pq.empty()){
                auto [movie_counter, id] = pq.top(); pq.pop();

                if(movie_counter > user_per_movie_temp[id]){
                    if(user_per_movie_temp[id])
                        pq.push({user_per_movie_temp[id], id});

                    continue;
                }

                seeds.push_back(id);
                coverage += movie_counter;
                for(auto u : user_list[id]){
                    if(user_covered[u])
                        continue;
                    
                    user_covered[u] = 1;
                    for(auto m : movie_list[u]){
                        user_per_movie_temp[m]--;
                    }
                }
            }

            return {coverage, seeds};
        }

        pair<int, vector<int>> compute_coverage_SG_Knapsack(int B){
            int n = movie_n;
            int coverage = 0;

            priority_queue<pair<double, int> > density;
            priority_queue<pair<int, int> > f;

            vector<int> user_per_movie_temp = user_per_movie;
            vector<int> user_covered(user_n, 0);

            for(int i = 0; i < n; i++){

                if(cost_movie[i] > B)
                    continue;
                density.push({(double)user_per_movie_temp[i]/cost_movie[i], i});
                f.push({user_per_movie_temp[i], i});
            }

            vector<int> S;
            vector<int> G;
            double budget = B;

            int f_G = 0, f_S = 0;

            while(budget > 0.0 && !density.empty() && !f.empty()){
                auto [f_v, v] = f.top(); f.pop();

                //Make sure v fits the budget
                while((cost_movie[v] > budget || f_v > user_per_movie_temp[v]) && !f.empty()){
                    if(cost_movie[v] > budget) {
                        f_v = f.top().first;
                        v = f.top().second;
                        f.pop();
                        continue;
                    }

                    if(user_per_movie_temp[v]) 
                        f.push({user_per_movie_temp[v], v});
                    
                    f_v = f.top().first;
                    v = f.top().second;
                    f.pop();
                }
                
                //Augmenting Greedy if possible
                if(budget >= cost_movie[v] && f_G + f_v > f_S){
                    S = G;
                    S.push_back(v);
                    f_S = f_G + f_v;
                }

                auto [rho_v, id] = density.top(); density.pop();

                while((cost_movie[id] > budget || rho_v > (double)user_per_movie_temp[id]/cost_movie[id]) && !density.empty()){
                    if(cost_movie[id] > budget){    
                        rho_v = density.top().first;
                        id = density.top().second;
                        density.pop();
                        continue;
                    }

                    if(user_per_movie_temp[id])
                        density.push({(double)user_per_movie_temp[id]/cost_movie[id], id});
                    

                    rho_v = density.top().first;
                    id = density.top().second;
                    density.pop();
                }

                if(cost_movie[id] > budget){
                    break;
                }

                G.push_back(id);
                f_G += user_per_movie_temp[id];
                budget -= cost_movie[id];

                for(auto u : user_list[id]){
                    if(user_covered[u])
                        continue;
                    
                    user_covered[u] = 1;
                    for(auto m : movie_list[u]){
                        user_per_movie_temp[m]--;
                    }
                }
            }

            coverage = f_S;

            return {coverage, S};
        }

        int compute_coverage_from_seeds(vector<int> seeds){
            set<int> n_users;

            for(int x : seeds){
                
                for(int y: user_list[x]){
                    n_users.insert(y);
                }
            }

            return (int)n_users.size() + seeds.size();
        }

        double compute_cost_of_seeds(vector<int> seeds){
            double total = 0.0;

            for(auto x : seeds){
                total += cost_movie[x];
            }

            return total;
        }

        void addRating(int user, int movie){
            user_list[movie].push_back(user);
            movie_list[user].push_back(movie);

            user_per_movie[movie]++;
        }

        void addMovieCost(int movie, double cost){
            cost_movie[movie] = cost;
        }
};

MovieRecommendation read_graph(string filename) {
    ifstream fin(filename);

    int n_users, n_movies, n_rating;

    fin >> n_users >> n_movies; //>> n_rating;

    //Uncomment for 1m
    // n_users = 6040;
    // n_movies = 3706;
    // n_rating = 1000209;


    MovieRecommendation mR(n_users, n_movies);

    int u, m, x;

    for(int i = 0; i < n_rating; i++){
        // fin >> u >> m;
        fin >> m >> u >> x;

        u -= n_movies ;

        mR.addRating(u, m);
    }

    return mR;
}

MovieRecommendation read_graph_costs(string filename){

    int n_users = 943, n_movies = 1682, n_rating = 100000;

    MovieRecommendation mR(n_users, n_movies);

    ifstream fin(filename);

    double c, x;

    int m, u;

    fin >> x >> x;

    for(int i = 0; i < n_movies; i++){
        fin >> x >> c >> x;
        
        mR.addMovieCost(i, c);
    }
    for(int i = n_movies; i < n_users + n_movies; i++){
        fin >> x >> x >> x;
    }

    for(int i = 0; i < n_rating; i++){
        fin >> m >> u >> x;

        u -= n_movies;

        mR.addRating(u, m);
    }

    return mR;    
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

vector<vector<int>> readOptimal_knapsack(string filename){
    ifstream fin(filename);

    vector<vector<int> > v;

    while(!fin.eof()){
        string s, w;
        v.push_back(vector<int>());
        getline(fin, s);

        istringstream sin(s);

        while(sin >> w){
            v.back().push_back(atoi(w.c_str()));
        }
    }

    return v;
}

int main(int argc, char * argv[]) {
    int k = atoi(argv[1]);
    MovieRecommendation mR = read_graph_costs(argv[2]);
    vector<vector<int> > opt = readOptimal_knapsack(argv[3]);

    // for(int i = k - 1; i < k; i++){

        cout << "--------------- K = " << k << "---------------" << endl;
        auto [val, seeds] = mR.compute_coverage_SG_Knapsack(k);

        cout << "Greedy coverage is: " << val << endl;
        cout << "Greedy cost: " << mR.compute_cost_of_seeds(seeds) << endl;
        cout << "Seeds: ";
        for(auto x : seeds){
            cout << x << ' ';
        }
        cout << endl;

        vector<int> tiptop = opt[0];
        // cout << tiptop[0] << endl;

        cout << "TipTop coverage: " << mR.compute_coverage_from_seeds(tiptop) << endl;
        cout << "TipTop cost: " << mR.compute_cost_of_seeds(tiptop) << endl;
        cout << "Seeds: ";
        for(auto x : tiptop){
            cout << x << ' ';
        }
        cout << endl;

        // vector<int> imm({104, 64, 44, 128, 171, 22, 113});

        // cout << "IMM Greedy coverage: " << mR.compute_coverage_from_seeds(imm) << endl;
        // cout << "Seeds: ";
        // for(auto x : imm){
        //     cout << x << ' ';
        // }
        // cout << endl;
    // }
    return 0;
}
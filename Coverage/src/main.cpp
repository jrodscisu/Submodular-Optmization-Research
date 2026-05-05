#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <random>

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
            user_per_movie.assign(movie_n, 0);
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

        pair<int, vector<int>> compute_coverage_OH(int k, set<int> hints){
            int n = movie_n;
            int coverage = 0;

            random_device rd;  
            mt19937 gen(rd()); 
            uniform_real_distribution<double> coin(0.0, 1.0); 


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

                int p = 0;

                for (auto x : hints){
                    if(user_per_movie_temp[x] > user_per_movie_temp[p]){
                        p = x;
                    }
                }

                double deltaG = user_per_movie_temp[id], deltaP = user_per_movie_temp[p];
                double Beta_i = deltaP/ (deltaG + deltaP);

                if(coin(gen) <= Beta_i){
                    id = p;
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

                if(hints.find(id) != hints.end()){
                    hints.erase(id);
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

            return (int)n_users.size();
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

    //100k_top5
    // n_users = 943;
    // n_movies = 1077;
    // n_rating = 4715;

    //1m_top5
    // n_users = 6040;
    // n_movies = 6040;
    // n_rating = 30200;

    //1m_top1
    // n_users = 6040;
    // n_movies = 6031;
    // n_rating = 6040;

    // //1m bottom5
    // n_users = 6040;
    // n_movies = 3174;
    // n_rating = 30200;
    
    //1m 25pmovies
    // n_users = 4557;
    // n_movies = 926;
    // n_rating = 35071;

    // 1m 10p movies

    // n_users = 3306;
    // n_movies = 370;
    // n_rating = 12987;

    //1m 50percentmovies

    // n_users = 5186;
    // n_movies = 1853;
    // n_rating = 76320;

    //1m 25percentmovies

    // n_users = 2695;
    // n_movies = 927;
    // n_rating = 11531;

    //1m 12percentmovies

    // n_users = 900;
    // n_movies = 464;
    // n_rating = 1850;

    //1m 1875percentmovies
    // n_users = 1822;
    // n_movies = 695;
    // n_rating = 5357;

    //1m 6.25%
    // n_users = 247;
    // n_movies = 232;
    // n_rating = 379;

    //1m 15%movies
    // n_users = 1250;
    // n_movies = 556;
    // n_rating = 2948;

    //1m top 3 bottom 3
    // n_users = 6040;
    // n_movies = 2753;
    // n_rating = 36230;

    //1m_top 1 bottom 1
    // n_users = 6040;
    // n_movies = 1878;
    // n_rating = 12080;

    //1m middle
    n_users = 6039;
    n_movies = 3663;
    n_rating = 717725;

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

MovieRecommendation read_graph_plus(string filename){

    int n_users, n_movies = 0;


    ifstream fin(filename);

    int m, u, edges;
    vector<pair<int, int> > ratings;

    fin >> n_users >> edges;

    int trash;

    while(edges--){
        fin >> m >> u >> trash;
        n_movies = max(n_movies, m + 1);
        ratings.push_back({m, u});
    }

    n_users -= n_movies;

    MovieRecommendation mR(n_users, n_movies);


    for(auto [m, u] : ratings){
        mR.addRating(u - n_movies, m);
    }

    return mR;    
}

vector<vector<int>> readOptimal(string filename, int n){
    ifstream fin;
    fin.open(filename);

    vector<vector<int>> v;

    int x;

    for(int i = n; i <= n; i++){
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

set<int> vec2set(vector<int> v){
    set<int> s(v.begin(), v.end());

    return s;
}

int main(int argc, char * argv[]) {
    int k = atoi(argv[1]);
    MovieRecommendation mR = read_graph_plus(argv[2]);
    cout << "ssldkfjlsd" << endl;
    vector<vector<int> > opt = readOptimal(argv[3], k);
    ofstream fout(string(argv[4]) + "_output.txt", std::ios_base::app);
    ofstream fout_gap("gaps.txt", std::ios_base::app);


    vector<int> results_opt;
    vector<int> results_greedy;
    vector<int> results_oh;

    for(int i = k - 1; i < k; i++){

        cout << "--------------- K = " << i + 1 << "---------------" << endl;
        fout << "--------------- K = " << i + 1 << "---------------" << endl;
        auto [val, seeds] = mR.compute_coverage_SG(i + 1);

        sort(seeds.begin(), seeds.end());
        val = mR.compute_coverage_from_seeds(seeds);

        results_greedy.push_back(val);

        cout << "Greedy coverage is: " << val << endl;
        fout << "Greedy coverage is: " << val << endl;
        cout << "Seeds: ";
        fout << "Seeds: ";
        for(auto x : seeds){
            cout << x << ' ';
            fout << x << ' ';
        }
        cout << endl;
        fout << endl;

        vector<int> tiptop = opt[0];
        // cout << tiptop[0] << endl;

        cout << "TipTop coverage: " << mR.compute_coverage_from_seeds(tiptop) << endl;
        fout << "TipTop coverage: " << mR.compute_coverage_from_seeds(tiptop) << endl;
        cout << "Seeds: ";
        fout << "Seeds: ";

        results_opt.push_back(mR.compute_coverage_from_seeds(tiptop));

        for(auto x : tiptop){
            cout << x << ' ';
            fout << x << endl;
        }
        cout << endl;
        fout << endl;

        fout_gap << argv[4] << ": " << results_opt.back() - results_greedy.back() << endl;

        vector<int> ohs;
        for(int j = 0; j < 50; j++){

            auto [c_, oh_seeds] = mR.compute_coverage_OH(i + 1, vec2set(opt[0]));

            ohs.push_back(mR.compute_coverage_from_seeds(oh_seeds));
        }

        cout << "OH coverage: " << ohs[25] << endl;
        fout << "OH coverage: " << ohs[25] << endl;
        results_oh.push_back(ohs[25]);
    }

    fout_gap.close();
    fout.close();

    // fout << "OPT: ";
    // for(int i = 0; i < k; i++){
    //     fout << "(" << i + 1 << ", " << results_opt[i] << ") ";
    // }
    // fout << endl;

    // fout << "Greedy: ";
    // for(int i = 0; i < k; i++){
    //     fout << "(" << i + 1 << ", " << results_greedy[i] << ") ";
    // }
    // fout << endl;

    // fout << "OH: ";
    // for(int i = 0; i < k; i++){
    //     fout << "(" << i + 1 << ", " << results_oh[i] << ") ";
    // }
    // fout << endl;
    return 0;
}
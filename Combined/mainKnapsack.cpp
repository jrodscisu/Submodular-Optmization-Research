#include <iostream>
#include <vector>
#include <queue>
#include <set>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <map>
#include <random>
#include <assert.h>

using namespace std;

#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>

double calculateSD(const std::vector<double>& data) {
    if (data.empty()) return 0.0;

    // 1. Calculate Mean
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    double mean = sum / data.size();

    // 2. Calculate Variance
    double variance = 0.0;
    for (double val : data) {
        variance += std::pow(val - mean, 2);
    }
    variance /= data.size(); // Use size() - 1 for sample standard deviation

    // 3. Return Square Root
    return std::sqrt(variance);
}


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

        pair<int, vector<int>> compute_coverage_OH_fair(int k, set<int> hints, double x_original, double R_original){
            int n = movie_n;
            int coverage = 0;

            double R_new = R_original;
            double x_new = x_original;


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

                vector<double> benefits;
                vector<int> hint_vec(hints.begin(), hints.end());

                int min_benefit = 1e9, max_benefit = -1e9;
                for(auto x : hint_vec){
                    min_benefit = min(min_benefit, user_per_movie_temp[x]);
                    max_benefit = max(max_benefit, user_per_movie_temp[x]);
                }

                R_new = max_benefit - min_benefit;

                for(auto x : hint_vec){
                    if(R_new == 0)
                        x_new = 0;
                    else
                        x_new = x_original * (R_original/R_new);
                    benefits.push_back(exp( x_new * user_per_movie_temp[x]));
                }

                discrete_distribution<int> hint_dist(benefits.begin(), benefits.end());

                p = hint_vec[hint_dist(gen)];

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

        pair<int, vector<int>> compute_coverage_OH_Norm(int k, set<int> hints, double x_original){
            int n = movie_n;
            int coverage = 0;

            double R;

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

                vector<double> benefits;
                vector<int> hint_vec(hints.begin(), hints.end());
                double min_benefit = 1e9, max_benefit = -1e9;
                for(auto x : hint_vec){
                    min_benefit = min(min_benefit, (double)user_per_movie_temp[x]);
                    max_benefit = max(max_benefit, (double)user_per_movie_temp[x]);
                    benefits.push_back((double)user_per_movie_temp[x]);
                }

                R = max_benefit - min_benefit;

                benefits.clear();

                for(auto x : hint_vec){
                    double norm_benefit;
                    if(R == 0)
                        norm_benefit = 0;
                    else
                        norm_benefit = ((double)user_per_movie_temp[x] - min_benefit) / R;
                    benefits.push_back(exp(x_original * norm_benefit ));
                }

                discrete_distribution<int> hint_dist(benefits.begin(), benefits.end());

                p = hint_vec[hint_dist(gen)];

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

        pair<int, vector<int>> compute_coverage_OH_SD(int k, set<int> hints, double x_original, double R_original){
            int n = movie_n;
            int coverage = 0;

            double R_new = R_original;
            double x_new = x_original;


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

                vector<double> benefits;
                vector<int> hint_vec(hints.begin(), hints.end());

                for(auto x : hint_vec){
                    benefits.push_back((double)user_per_movie_temp[x]);
                }

                R_new = calculateSD(benefits);

                benefits.clear();

                for(auto x : hint_vec){
                    if(R_new == 0)
                        x_new = 0;
                    else
                        x_new = x_original * (R_original/R_new);
                    benefits.push_back(exp( x_new * user_per_movie_temp[x]));
                }

                discrete_distribution<int> hint_dist(benefits.begin(), benefits.end());

                p = hint_vec[hint_dist(gen)];

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
        tuple<int, vector<int>, vector<tuple<int, double, double, int, int, double, double>>> compute_coverage_OH_Norm2(int k, set<int> hints, double x_original){
           //******************************************************** */
            vector<int> optimal_vec(hints.begin(), hints.end());
            vector<tuple<int, double, double, int, int, double, double>> history;
            int OPT = compute_coverage_from_seeds(optimal_vec); 
           //********************************************************* */ 
            int n = movie_n;
            int coverage = 0;

            double R;

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

                vector<double> benefits;
                vector<int> hint_vec(hints.begin(), hints.end());
                double min_benefit = 1e9, max_benefit = -1e9;
                for(auto x : hint_vec){
                    min_benefit = min(min_benefit, (double)user_per_movie_temp[x]);
                    max_benefit = max(max_benefit, (double)user_per_movie_temp[x]);
                    benefits.push_back((double)user_per_movie_temp[x]);
                }

                R = max_benefit - min_benefit;

                benefits.clear();

                for(auto x : hint_vec){
                    double norm_benefit;
                    if(R == 0)
                        norm_benefit = 0;
                    else
                        norm_benefit = ((double)user_per_movie_temp[x] - min_benefit) / R;
                    benefits.push_back(exp(x_original * norm_benefit ));
                }

                discrete_distribution<int> hint_dist(benefits.begin(), benefits.end());

                p = hint_vec[hint_dist(gen)];

                double deltaG = user_per_movie_temp[id], deltaP = user_per_movie_temp[p];
                //********************************************************* */
                // Calculate current residual gap
                vector<int> current_seeds = seeds;
                seeds.push_back(id);
                deltaG = compute_coverage_from_seeds(seeds) - compute_coverage_from_seeds(current_seeds);
                seeds.pop_back();
                seeds.push_back(p);
                deltaP = compute_coverage_from_seeds(seeds) - compute_coverage_from_seeds(current_seeds);
                seeds.pop_back();
                double residual_gap = OPT - compute_coverage_from_seeds(current_seeds);

                // Calculate rho values (with a guard against division by zero if residual_gap <= 0)
                double rho_g = (residual_gap > 0) ? (deltaG * k) / residual_gap : 0.0;
                double rho_p = (residual_gap > 0) ? (deltaP * k) / residual_gap : 0.0;

                // Print to console in CSV format for easy plotting
                // cout << seeds.size() + 1 << "," << rho_g << "," << rho_p << endl;
                history.push_back({(int)seeds.size() + 1, rho_g, rho_p, id, p, deltaG, deltaP});
                //*********************************************************** */
                double Beta_i = deltaP/ (deltaG + deltaP);

                // if(coin(gen) <= Beta_i){
                    // id = p;
                    // coverage += deltaP;
                // }else{
                    coverage += deltaG;
                // }

                seeds.push_back(id);

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

            if(coverage > OPT){
                cout << "Warning: Coverage exceeds OPT! Coverage: " << coverage << ", OPT: " << OPT << endl;
            }

            return {coverage, seeds, history};
        }
        pair<int, vector<int>> compute_coverage_OH_Knapsack(double B, set<int> hints){
            int n = movie_n;
            int coverage = 0;

            random_device rd;  
            mt19937 gen(rd()); 
            uniform_real_distribution<double> coin(0.0, 1.0);

            priority_queue<pair<double, int> > density;
            priority_queue<pair<int, int> > f;

            vector<int> user_per_movie_temp = user_per_movie;
            vector<int> user_covered(user_n, 0);

            for(int i = 0; i < n; i++){

                if(B - cost_movie[i] < 1e-6)
                    continue;
                density.push({(double)user_per_movie_temp[i]/cost_movie[i], i});
                f.push({user_per_movie_temp[i], i});
            }

            vector<vector<int>> augmentations;

            vector<int> S;
            vector<int> G;
            double budget = B;

            int f_G = 0, f_S = 0;

            while(budget > 1e-6 && !density.empty() && !f.empty()){
                auto [dg, g] = density.top();

                if(budget - cost_movie[g] < 1e-6 || user_per_movie_temp[g] == 0 || (double)user_per_movie_temp[g]/cost_movie[g] < dg){
                    density.pop();
                    if(budget - cost_movie[g] > 1e-6 && user_per_movie_temp[g] > 0)
                        density.push({(double)user_per_movie_temp[g]/cost_movie[g], g});
                    continue;
                }

                auto [f_v, v] = f.top();

                if(budget - cost_movie[v] < 1e-6 || user_per_movie_temp[v] == 0 || f_v < user_per_movie_temp[v]){
                    f.pop();
                    if(budget - cost_movie[v] > 1e-6 && user_per_movie_temp[v] > 0)
                        f.push({user_per_movie_temp[v], v});
                    continue;
                }

                G.push_back(v);
                augmentations.push_back(G);
                G.pop_back();

                int selected = g;

                pair<double, int> best_prediction = {0.0, -1};

                set<int> unavailable_hints;

                for(auto h : hints){
                    if(user_per_movie_temp[h] == 0 || cost_movie[h] > budget){
                        unavailable_hints.insert(h);
                        continue;
                    }
                    double density_h = (double)user_per_movie_temp[h]/cost_movie[h];
                    if(density_h > best_prediction.first){
                        best_prediction = {density_h, h};
                    }
                }

                for(auto h : unavailable_hints){
                    hints.erase(h);
                }


                if(hints.size() > 0){
                    vector<int> predictions(hints.begin(), hints.end());
                    uniform_real_distribution<int> coin_p(0, predictions.size() - 1);
                    
                    best_prediction.second = predictions[coin_p(gen)];
                    best_prediction.first = (double)user_per_movie_temp[best_prediction.second]/cost_movie[best_prediction.second];
                }
                
                if(best_prediction.second != -1){
                    double d_p = best_prediction.first;
                    double c_p = cost_movie[best_prediction.second];
                    double c_g = cost_movie[g];
                    double Beta_i = (d_p/c_p) / ((d_p/c_p) + (dg/c_g));

                    if(coin(gen) <= Beta_i){
                        cout << "Selected hint " << best_prediction.second << " over movie " << g << " with probability " << Beta_i << endl;
                        selected = best_prediction.second;
                    }
                }

                G.push_back(selected);

                budget -= cost_movie[selected];

                for(auto u : user_list[selected]){
                    if(user_covered[u])
                        continue;
                    
                    user_covered[u] = 1;
                    for(auto m : movie_list[u]){
                        user_per_movie_temp[m]--;
                    }
                }
            }

            pair<int, vector<int>> best_augmentation = {compute_coverage_from_seeds(G), G};

            for(auto aug : augmentations){
                int aug_coverage = compute_coverage_from_seeds(aug);
                if(aug_coverage > best_augmentation.first){
                    best_augmentation = {aug_coverage, aug};
                }
            }


            return best_augmentation;
        }
        pair<int, vector<int>> compute_coverage_SG_Knapsack(double B){
            int n = movie_n;
            int coverage = 0;

            priority_queue<pair<double, int> > density;
            priority_queue<pair<int, int> > f;

            vector<int> user_per_movie_temp = user_per_movie;
            vector<int> user_covered(user_n, 0);

            for(int i = 0; i < n; i++){

                if(B - cost_movie[i] < 1e-6)
                    continue;
                density.push({(double)user_per_movie_temp[i]/cost_movie[i], i});
                f.push({user_per_movie_temp[i], i});
            }

            vector<vector<int>> augmentations;

            vector<int> S;
            vector<int> G;
            double budget = B;

            int f_G = 0, f_S = 0;

            while(budget > 1e-6 && !density.empty() && !f.empty()){
                auto [dg, g] = density.top();

                if(budget - cost_movie[g] < 1e-6 || user_per_movie_temp[g] == 0 || (double)user_per_movie_temp[g]/cost_movie[g] < dg){
                    density.pop();
                    if(budget - cost_movie[g] > 1e-6 && user_per_movie_temp[g] > 0)
                        density.push({(double)user_per_movie_temp[g]/cost_movie[g], g});
                    continue;
                }

                auto [f_v, v] = f.top();

                if(budget - cost_movie[v] < 1e-6 || user_per_movie_temp[v] == 0 || f_v < user_per_movie_temp[v]){
                    f.pop();
                    if(budget - cost_movie[v] > 1e-6 && user_per_movie_temp[v] > 0)
                        f.push({user_per_movie_temp[v], v});
                    continue;
                }

                G.push_back(v);
                augmentations.push_back(G);
                G.pop_back();

                G.push_back(g);

                budget -= cost_movie[g];

                for(auto u : user_list[g]){
                    if(user_covered[u])
                        continue;
                    
                    user_covered[u] = 1;
                    for(auto m : movie_list[u]){
                        user_per_movie_temp[m]--;
                    }
                }
            }

            pair<int, vector<int>> best_augmentation = {compute_coverage_from_seeds(G), G};

            for(auto aug : augmentations){
                int aug_coverage = compute_coverage_from_seeds(aug);
                if(aug_coverage > best_augmentation.first){
                    best_augmentation = {aug_coverage, aug};
                }
            }


            return best_augmentation;
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

MovieRecommendation read_graph_knapsack(string filename, string filename_cost){

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

    cout << n_movies << ' ' << n_users << endl;

    MovieRecommendation mR(n_users, n_movies);

    for(auto [m, u] : ratings){
        mR.addRating(u - n_movies, m);
    }
    
    fin.close();

    ifstream fin_cost(filename_cost);

    for(int i = 0; i < n_movies; i++){
        double cost;
        fin_cost >> cost;

        // cout << cost << ' ';
        mR.addMovieCost(i, cost);
    }
    cout << endl;

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

    cout << n_users << endl;

    MovieRecommendation mR(n_users, n_movies);


    for(auto [m, u] : ratings){
        mR.addRating(u - n_movies, m);
    }

    return mR;    
}

vector<set<int>> readOptimal(string filename, int n){
    ifstream fin;
    fin.open(filename);

    vector<set<int>> v;

    int x;
    string s;

    for(int i = 3; i <= n; i++){
        v.push_back(set<int>());
        getline(fin, s);        
        stringstream sin(s);

        while(sin >> x){
            v.back().insert(x);
        }
    }

    fin.close();

    return v;
}

vector<set<int>> readOptimal_knapsack(string filename){
    ifstream fin(filename);

    vector<set<int> > v;

    while(!fin.eof()){
        string s, w;
        v.push_back(set<int>());
        getline(fin, s);
        
        istringstream sin(s);

        while(sin >> w){
            v.back().insert(atoi(w.c_str()));
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
    MovieRecommendation mR = read_graph_knapsack(argv[2], argv[3]);
    vector<set<int>> opt = readOptimal_knapsack(argv[4]);
    ofstream fout("output_Norm.txt", std::ios_base::app);
    ofstream rhoout("output_rho_knapsack.txt");


    vector<int> results_opt;
    vector<int> results_greedy;
    vector<int> results_oh;

    rhoout << "K,Step,rho_g,rho_p,id,p,deltaG,deltaP" << endl;


    for(int i = 0; i < 1; i++){

        cout << "--------------- K = " << k << "---------------" << endl;
        fout << "--------------- K = " << k << "---------------" << endl;
        auto [val, seeds] = mR.compute_coverage_SG_Knapsack(k);

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

        set<int> hints = opt[0];

        double avg_rho_g = 0.0, avg_rho_p = 0.0;
        vector<int> ohs;
        for(int j = 0; j < 50; j++){

            auto [coverage, seeds] = mR.compute_coverage_OH_Knapsack(k, hints);

            ohs.push_back(mR.compute_coverage_from_seeds(seeds));
        
            // for(auto [k_val, rho_g, rho_p, id, p, deltaG, deltaP] : oh_history){
            //     rhoout << i + 1 << "," << k_val << "," << rho_g << "," << rho_p << "," << id << "," << p << "," << deltaG << "," << deltaP << endl;
            //     avg_rho_g += rho_g;
            //     avg_rho_p += rho_p;
            // }
            // avg_rho_g /= (double)oh_history.size();
            // avg_rho_p /= (double)oh_history.size();
        }

        cout << "OPT: ";
        for(auto x : hints){
            cout << x << ' ';
        }
        cout << endl;

        sort(ohs.begin(), ohs.end());

        cout << "OPT coverage for k = " << k << ": " << mR.compute_coverage_from_seeds(vector<int>(hints.begin(), hints.end())) << endl;
        cout << "OH coverage for k = " << k << ": " << ohs[0] << " to " << ohs[ohs.size()/2] << " to " << ohs[ohs.size()-1] << endl;
        cout << "Average rho_g for k = " << k << ": " << avg_rho_g << endl;
        cout << "Average rho_p for k = " << k << ": " << avg_rho_p << endl;
        fout << "OH coverage for k = " << k << ": " << ohs[0] << " to " << ohs[ohs.size()/2] << " to " << ohs[ohs.size()-1] << endl;
        results_oh.push_back(ohs[ohs.size()/2]);
        
    }

    fout.close();
    rhoout.close();
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
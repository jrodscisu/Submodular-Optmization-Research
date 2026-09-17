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

        //  pair<int, vector<int>> compute_coverage_OH_SD(int k, set<int> hints, double x_original, double R_original){
        //     int n = movie_n;
        //     int coverage = 0;

        //     double R_new = R_original;
        //     double x_new = x_original;


        //     random_device rd;  
        //     mt19937 gen(rd()); 
        //     uniform_real_distribution<double> coin(0.0, 1.0);

        //     priority_queue<pair<int, int> > pq;

        //     vector<int> user_per_movie_temp = user_per_movie;
        //     vector<int> user_covered(user_n, 0);

        //     for(int i = 0; i < n; i++){
        //         pq.push({user_per_movie_temp[i], i});
        //     }

        //     vector<int> seeds;

        //     while((int)seeds.size() < k && !pq.empty()){
        //         auto [movie_counter, id] = pq.top(); pq.pop();

        //         if(movie_counter > user_per_movie_temp[id]){
        //             if(user_per_movie_temp[id])
        //                 pq.push({user_per_movie_temp[id], id});

        //             continue;
        //         }

        //         int p = 0;

        //         vector<double> benefits;
        //         vector<int> hint_vec(hints.begin(), hints.end());

        //         for(auto x : hint_vec){
        //             benefits.push_back((double)user_per_movie_temp[x]);
        //         }

        //         R_new = calculateSD(benefits);

        //         benefits.clear();

        //         for(auto x : hint_vec){
        //             if(R_new == 0)
        //                 x_new = 0;
        //             else
        //                 x_new = x_original * (R_original/R_new);
        //             benefits.push_back(exp( x_new * user_per_movie_temp[x]));
        //         }

        //         discrete_distribution<int> hint_dist(benefits.begin(), benefits.end());

        //         p = hint_vec[hint_dist(gen)];

        //         double deltaG = user_per_movie_temp[id], deltaP = user_per_movie_temp[p];
        //         double Beta_i = deltaP/ (deltaG + deltaP);

        //         if(coin(gen) <= Beta_i){
        //             id = p;
        //         }

        //         seeds.push_back(id);

        //         coverage += movie_counter;
        //         for(auto u : user_list[id]){
        //             if(user_covered[u])
        //                 continue;
                    
        //             user_covered[u] = 1;
        //             for(auto m : movie_list[u]){
        //                 user_per_movie_temp[m]--;
        //             }
        //         }

        //         if(hints.find(id) != hints.end()){
        //             hints.erase(id);
        //         }
        //     }

        //     return {coverage, seeds};
        // }

        // pair<int, vector<int>> compute_coverage_OH_Norm(int k, set<int> hints, double x_original){
        //     int n = movie_n;
        //     int coverage = 0;

        //     double R;

        //     random_device rd;  
        //     mt19937 gen(rd()); 
        //     uniform_real_distribution<double> coin(0.0, 1.0);

        //     priority_queue<pair<int, int> > pq;

        //     vector<int> user_per_movie_temp = user_per_movie;
        //     vector<int> user_covered(user_n, 0);

        //     for(int i = 0; i < n; i++){
        //         pq.push({user_per_movie_temp[i], i});
        //     }

        //     vector<int> seeds;

        //     while((int)seeds.size() < k && !pq.empty()){
        //         auto [movie_counter, id] = pq.top(); pq.pop();

        //         if(movie_counter > user_per_movie_temp[id]){
        //             if(user_per_movie_temp[id])
        //                 pq.push({user_per_movie_temp[id], id});

        //             continue;
        //         }

        //         int p = 0;

        //         vector<double> benefits;
        //         vector<int> hint_vec(hints.begin(), hints.end());
        //         double min_benefit = 1e9, max_benefit = -1e9;
        //         for(auto x : hint_vec){
        //             min_benefit = min(min_benefit, (double)user_per_movie_temp[x]);
        //             max_benefit = max(max_benefit, (double)user_per_movie_temp[x]);
        //             benefits.push_back((double)user_per_movie_temp[x]);
        //         }

        //         R = max_benefit - min_benefit;

        //         benefits.clear();

        //         for(auto x : hint_vec){
        //             double norm_benefit;
        //             if(R == 0)
        //                 norm_benefit = 0;
        //             else
        //                 norm_benefit = ((double)user_per_movie_temp[x] - min_benefit) / R;
        //             benefits.push_back(exp(x_original * norm_benefit ));
        //         }

        //         discrete_distribution<int> hint_dist(benefits.begin(), benefits.end());

        //         p = hint_vec[hint_dist(gen)];

        //         double deltaG = user_per_movie_temp[id], deltaP = user_per_movie_temp[p];
        //         double Beta_i = deltaP/ (deltaG + deltaP);

        //         if(coin(gen) <= Beta_i){
        //             id = p;
        //         }

        //         seeds.push_back(id);

        //         coverage += movie_counter;
        //         for(auto u : user_list[id]){
        //             if(user_covered[u])
        //                 continue;
                    
        //             user_covered[u] = 1;
        //             for(auto m : movie_list[u]){
        //                 user_per_movie_temp[m]--;
        //             }
        //         }

        //         if(hints.find(id) != hints.end()){
        //             hints.erase(id);
        //         }
        //     }

        //     return {coverage, seeds};
        // }

        //  pair<int, vector<int>> compute_coverage_OH_SD(int k, set<int> hints, double x_original, double R_original){
        //     int n = movie_n;
        //     int coverage = 0;

        //     double R_new = R_original;
        //     double x_new = x_original;


        //     random_device rd;  
        //     mt19937 gen(rd()); 
        //     uniform_real_distribution<double> coin(0.0, 1.0);

        //     priority_queue<pair<int, int> > pq;

        //     vector<int> user_per_movie_temp = user_per_movie;
        //     vector<int> user_covered(user_n, 0);

        //     for(int i = 0; i < n; i++){
        //         pq.push({user_per_movie_temp[i], i});
        //     }

        //     vector<int> seeds;

        //     while((int)seeds.size() < k && !pq.empty()){
        //         auto [movie_counter, id] = pq.top(); pq.pop();

        //         if(movie_counter > user_per_movie_temp[id]){
        //             if(user_per_movie_temp[id])
        //                 pq.push({user_per_movie_temp[id], id});

        //             continue;
        //         }

        //         int p = 0;

        //         vector<double> benefits;
        //         vector<int> hint_vec(hints.begin(), hints.end());

        //         for(auto x : hint_vec){
        //             benefits.push_back((double)user_per_movie_temp[x]);
        //         }

        //         R_new = calculateSD(benefits);

        //         benefits.clear();

        //         for(auto x : hint_vec){
        //             if(R_new == 0)
        //                 x_new = 0;
        //             else
        //                 x_new = x_original * (R_original/R_new);
        //             benefits.push_back(exp( x_new * user_per_movie_temp[x]));
        //         }

        //         discrete_distribution<int> hint_dist(benefits.begin(), benefits.end());

        //         p = hint_vec[hint_dist(gen)];

        //         double deltaG = user_per_movie_temp[id], deltaP = user_per_movie_temp[p];
        //         double Beta_i = deltaP/ (deltaG + deltaP);

        //         if(coin(gen) <= Beta_i){
        //             id = p;
        //         }

        //         seeds.push_back(id);

        //         coverage += movie_counter;
        //         for(auto u : user_list[id]){
        //             if(user_covered[u])
        //                 continue;
                    
        //             user_covered[u] = 1;
        //             for(auto m : movie_list[u]){
        //                 user_per_movie_temp[m]--;
        //             }
        //         }

        //         if(hints.find(id) != hints.end()){
        //             hints.erase(id);
        //         }
        //     }

        //     return {coverage, seeds};
        // }

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
    MovieRecommendation mR = read_graph_plus(argv[2]);
    vector<set<int>> opt = readOptimal(argv[3], k);
    ofstream fout("output_Norm.txt", std::ios_base::app);
    ofstream rhoout("output_rho.txt", std::ios_base::app);


    vector<int> results_opt;
    vector<int> results_greedy;
    vector<int> results_oh;

    rhoout << "K,Step,rho_g,rho_p,id,p,deltaG,deltaP" << endl;


    for(int i = 2; i < k; i++){

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

        set<int> hints = opt[i - 2];

        // for(int z = 0; z <= 30; z++){
        //     double x = -7.50 + (double)z * 0.5;
        //     vector<int> ohs;
        //     for(int j = 0; j < 50; j++){

        //         auto [c_, oh_seeds] = mR.compute_coverage_OH_Norm(i + 1, hints, x);

        //         ohs.push_back(mR.compute_coverage_from_seeds(oh_seeds));
        //     }
        //     cout << "OH coverage for k = " << i + 1 << ", x = " << x << ": " << ohs[25] << endl;
        //     fout << "OH coverage for k = " << i + 1 << ", x = " << x << ": " << ohs[25] << endl;
        //     results_oh.push_back(ohs[25]);
        // }

        for(int z = 15; z <= 15; z++){
            double x = -7.50 + (double)z * 0.5;
            vector<int> ohs;
            for(int j = 0; j < 50; j++){

                auto [c_, oh_seeds, oh_history] = mR.compute_coverage_OH_Norm2(i + 1, hints, x);

                ohs.push_back(mR.compute_coverage_from_seeds(oh_seeds));
            
                for(auto [k_val, rho_g, rho_p, id, p, deltaG, deltaP] : oh_history){
                    rhoout << i + 1 << "," << k_val << "," << rho_g << "," << rho_p << "," << id << "," << p << "," << deltaG << "," << deltaP << endl;
                }
            }
            cout << "OPT coverage for k = " << i + 1 << ": " << mR.compute_coverage_from_seeds(vector<int>(hints.begin(), hints.end())) << endl;
            cout << "OH coverage for k = " << i + 1 << ", x = " << x << ": " << ohs[25] << endl;
            fout << "OH coverage for k = " << i + 1 << ", x = " << x << ": " << ohs[25] << endl;
            results_oh.push_back(ohs[25]);
        }
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
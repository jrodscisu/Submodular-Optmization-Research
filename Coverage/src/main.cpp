#include <iostream>
#include <fstream>

using namespace std;

class MovieRecommendation{
    public:
        int user_n, movie_n;
        vector<int> covered;
        vector<int> user_per_movie;
        vector<vector<int>> user_list;
        vector<vector<int>> movie_list;

        MovieRecommendation(int user, int movie){
            user_n = user;
            movie_n = movie;
            covered.assign(user_n, 0);
            user_per_movie.assign(movie_n, 0);
            user_list.assign(movie_n, vector<int>());
            movie_list.assign(user_n, vector<int>());
        }

        pair<int, vector<int>> compute_coverage_SG(int k){
            int n = covered.size();
            int coverage = 0;

            priority_queue<pair<int, int> > pq;

            for(int i = 0; i < n; i++){
                pq.push({i, user_per_movie[i]});
            }

            vector<int> seeds;

            while((int)seeds.size() < k && !pq.empty()){
                auto [id, movie_counter] = pq.top(); pq.pop();

                if(movie_counter > user_per_movie[id]){
                    if(user_per_movie[id])
                        pq.push({user_per_movie[id], id});

                    continue;
                }

                seeds.push_back(id);
                coverage += user_per_movie[id];
                for(auto u : user_list[id]){
                    for(auto m : movie_list[u]){
                        user_per_movie[m]--;
                    }
                }
            }

            return {coverage, seeds};
        }

        void addRating(int user, int movie){
            user_list[movie].push_back(user);
            movie_list[user].push_back(movie);

            user_per_movie[movie]++;
        }
};

MovieRecommendation read_graph(string filename) {
    ifstream fin(filename);

    int n_users, n_movies, n_rating;

    fin >> n_users >> n_movies >> n_rating;

    MovieRecommendation mR(n_users, n_movies);

    int u, m;

    for(int i = 0; i < n_rating; i++){
        fin >> u >> m;

        mR.addRating(u, m);
    }

    return mR;
}

int main(int argc, char * argv[]) {
    int k = atoi(argv[1]);
    MovieRecommendation mR = read_graph(argv[2]);

    auto [val, seeds] = mR.compute_coverage_SG(k);

    cout << "Greedy coverage is: " << val << endl;
    cout << "Seeds: ";
    for(auto x : seeds){
        cout << x << ' ';
    }
    cout << endl;

    return 0;
}
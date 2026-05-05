#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <set>
#include <queue>
#include <map>

using namespace std;

int main(int argc, char * argv[]) {
    string in_file = argv[1];
    string out_file = argv[2];
    double per_movie = stod(argv[3]);

    ifstream fin(in_file);
    ofstream fout(out_file);
    
    vector<int> ratings(5, 0);
    vector<map<int, int>> ratings_unique(5);
    set<int> u_m;

    map<int, int> m_user, m_movie;


    int cont = 0;
    int u, m, r;

    vector<pair<int, int> > e;

    while(!fin.eof()){

        fin >> m >> u >> r >> ws;

        ratings_unique[r-1][m]++;

        u_m.insert(m);

        e.push_back({m, u});

    }

    set<int> niche_m;
    vector<vector<pair<int, int>>> per_rating_sorted; 

    for(int i = 0; i < 5; i++){
        vector<pair<int, int> > movies;

        for(auto [k, v] : ratings_unique[i]){
            movies.push_back({v, k});
        }

        sort(movies.begin(), movies.end());
        per_rating_sorted.push_back(movies);
    }

    priority_queue<pair<int, pair<int, int>>, vector<pair<int,pair<int, int > >>, greater<pair<int, pair<int, int> > >> pq;

    for(int i = 0; i < 5; i++){
        pq.push({per_rating_sorted[i][0].first, {per_rating_sorted[i][0].second, i}});
    }

    vector<int> is(5, 0);

    while((int)niche_m.size() < int(per_movie * u_m.size())){
        auto [ x, ii] = pq.top();pq.pop();

        niche_m.insert(ii.first);
        int j = is[ii.second]++;
        if(j < (int)per_rating_sorted[ii.second].size()){
            pq.push({per_rating_sorted[ii.second][j].first, {per_rating_sorted[ii.second][j].second, ii.second}});
        }
    }


    cont = 0;

    vector<pair<int, int>> last_e;

    for(auto [m, u] : e){
        if(niche_m.find(m) == niche_m.end())
            continue;

        if(m_movie.find(m) == m_movie.end()){
            m_movie.insert({m, m_movie.size()});
        }

        if(m_user.find(u) == m_user.end()){
            m_user.insert({u, m_user.size()});
        }

        last_e.push_back({m_movie[m], m_user[u]});

        cont++;
    }

    fout << m_user.size() + m_movie.size() << ' ' << cont << endl;

    cout << m_user.size() << ' ' << m_movie.size() << endl;
    
    for(auto [m, u] : last_e){
        fout << m << ' ' << u + m_movie.size() << ' ' << 1 << endl;
    }

    return 0;
}
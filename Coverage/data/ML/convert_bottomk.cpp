#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <queue>
#include <map>

using namespace std;

int main(int argc, char * argv[]) {
    string in_file = argv[1];
    string out_file = argv[2];
    int top_k = atoi(argv[3]);

    ifstream fin(in_file);
    ofstream fout(out_file);

    map<int, int> m_user, m_movie;
    vector<priority_queue<pair<int, int>>> g;
    int cont = 0;
    int u, m, r;
    while(!fin.eof()){

        fin >> m >> u >> r >> ws;

        if(m_user.find(u) == m_user.end()){
            m_user.insert({u, m_user.size()});
            g.push_back(priority_queue<pair<int, int>> ());
        }

        if((int)g[m_user[u]].size() == top_k){
            if (g[m_user[u]].top().first > r){
                g[m_user[u]].pop();

                if(m_movie.find(m) == m_movie.end()){
                    m_movie.insert({m, m_movie.size()});
                }

                g[m_user[u]].push({r, m_movie[m]});
            }
            continue;
        }else {
            if(m_movie.find(m) == m_movie.end()){
                m_movie.insert({m, m_movie.size()});
            }

            g[m_user[u]].push({r, m_movie[m]});
        }
        
        cont++;
    }

    fout << m_user.size() + m_movie.size() << ' ' << cont << endl;

    cout << m_user.size() << ' ' << m_movie.size() << endl;
    
    for(auto [key, user] : m_user){
        while(!g[user].empty()){
            auto [r_, movie] = g[user].top(); g[user].pop();

            fout << movie << ' ' << user + (int)m_movie.size() << " " << 1 << endl;
        }
    }

    return 0;
}
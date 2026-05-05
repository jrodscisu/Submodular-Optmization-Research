#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <queue>
#include <set>
#include <map>

using namespace std;

int main(int argc, char * argv[]) {
    string in_file = argv[1];
    string out_file = argv[2];
    int top_k = atoi(argv[3]);

    ifstream fin(in_file);
    ofstream fout(out_file);

    map<int, int> m_user, m_movie;
    vector<priority_queue<pair<int, int>>> g_bottom;
    vector<priority_queue<pair<int, int>, vector<pair<int, int> > , greater<pair<int, int>> >> g_top;
    int cont = 0;
    int u, m, r;
    while(!fin.eof()){

        fin >> m >> u >> r >> ws;

        if(m_user.find(u) == m_user.end()){
            m_user.insert({u, m_user.size()});
            g_bottom.push_back(priority_queue<pair<int, int>> ());
            g_top.push_back(priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> >  > ());
        }

        if((int)g_bottom[m_user[u]].size() == top_k){
            if (g_bottom[m_user[u]].top().first > r){
                g_bottom[m_user[u]].pop();

                if(m_movie.find(m) == m_movie.end()){
                    m_movie.insert({m, m_movie.size()});
                }

                g_bottom[m_user[u]].push({r, m_movie[m]});
            }
        }else {
            if(m_movie.find(m) == m_movie.end()){
                m_movie.insert({m, m_movie.size()});
            }

            g_bottom[m_user[u]].push({r, m_movie[m]});
        }

        if((int)g_top[m_user[u]].size() == top_k){
            if (g_top[m_user[u]].top().first < r){
                g_top[m_user[u]].pop();

                if(m_movie.find(m) == m_movie.end()){
                    m_movie.insert({m, m_movie.size()});
                }

                g_top[m_user[u]].push({r, m_movie[m]});
            }
            continue;
        }else {
            if(m_movie.find(m) == m_movie.end()){
                m_movie.insert({m, m_movie.size()});
            }

            g_top[m_user[u]].push({r, m_movie[m]});
        }
        
        cont++;
    }


    cout << m_user.size() << ' ' << m_movie.size() << endl;

    set<pair<int, int> > output;
    
    for(auto [key, user] : m_user){
        while(!g_bottom[user].empty()){
            auto [r_, movie] = g_bottom[user].top(); g_bottom[user].pop();

            output.insert({movie, user + (int)m_movie.size()});

        }
        while(!g_top[user].empty()){
            auto [r_, movie] = g_top[user].top(); g_top[user].pop();

            output.insert({movie, user + (int)m_movie.size()});
        }
    }

    fout << m_user.size() + m_movie.size() << ' ' << output.size() << endl;

    for(auto [a, b] : output){
        fout << a << ' ' << b << ' ' << 1 << endl;
    }

    return 0;
}
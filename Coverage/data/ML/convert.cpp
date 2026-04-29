#include <iostream>
#include <fstream>
#include <map>

using namespace std;

int main(int argc, char * argv[]) {
    string in_file = argv[1];
    string out_file = argv[2];

    ifstream fin(in_file);
    ofstream fout(out_file);

    map<int, int> m_user, m_movie;
    vector<pair<int, int> > e;
    int cont;
    int u, m;
    while(!fin.eof()){

        fin >> u >> m >> ws;

        if(m_user.find(u) == m_user.end()){
            m_user.insert({u, m_user.size()});
        }
        if(m_movie.find(m) == m_movie.end()){
            m_movie.insert({m, m_movie.size()});
        }

        e.push_back({m_user[u], m_movie[m]});
        cont++;
    }

    fout << m_user.size() << ' ' << m_movie.size() << ' ' << cont << endl;

    sort(e.begin(), e.end());
    
    for(auto [a, b] : e){
        fout << a << ' ' << b << endl;
    }

    return 0;
}
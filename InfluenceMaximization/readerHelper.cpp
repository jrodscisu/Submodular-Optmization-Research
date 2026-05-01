#include <bits/stdc++.h>

using namespace std;

int main() {

    ifstream fin;

    fin.open("data/Wiki-Vote.txt");

    map<int, int> m;
    vector<int> degree;
    vector<pair<int, int> > e;

    while(!fin.eof()){
        int u, v;

        fin >> u >> v;

        if(m.find(u) == m.end()){
            m.insert({u, m.size()});
            degree.push_back(0);
        }

        if(m.find(v) == m.end()){
            m.insert({v, m.size()});
            degree.push_back(0);
        }

        degree[m[v]]++;

        e.push_back(make_pair(m[u], m[v]));
    }

    cout << m.size() << ' ' << e.size() << endl;

    for(auto [u, v] : e){
        cout << u << ' ' << v << ' ' << 0.01 << endl;
    }

    return 0;
}
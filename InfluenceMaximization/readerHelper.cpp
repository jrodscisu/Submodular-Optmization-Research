#include <bits/stdc++.h>

using namespace std;

int main() {

    ifstream fin;

    fin.open("data/CA-GrQc.txt");

    map<int, int> m;
    vector<pair<int, int> > e;

    while(!fin.eof()){
        int u, v;

        fin >> u >> v;

        if(m.find(u) == m.end()){
            m.insert({u, m.size()});
        }

        if(m.find(v) == m.end()){
            m.insert({v, m.size()});
        }

        e.push_back(make_pair(m[u], m[v]));
    }

    cout << m.size() << ' ' << e.size() << endl;

    for(auto [u, v] : e){
        cout << u << ' ' << v << endl;
    }

    return 0;
}
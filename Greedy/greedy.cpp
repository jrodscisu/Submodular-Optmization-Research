#include <iostream>
#include "greedy.h"

#define eps 1e-9

using namespace std;

int max_marginal_gain_element(int N, int K, set<int> &S){
    
    double max = INT_MIN;
    int g_max = -1;
    
    for(int i = 0; i < N; i++){
        if(S.find(i) != S.end())
            continue;
        
        double m_g = marginal_gain(N, K, S, i);
        if(m_g - max > eps){
            max = m_g;
            g_max = i;
        }
    }

    return g_max;
}

pair<set<int>, int> greedy(int N, int K){
    set<int> S;
    
    while((int)S.size() < K){
        int greedy_choice = max_marginal_gain_element(N, K, S);

        S.insert(greedy_choice);
    }


    return make_pair(S, zfun(N, K, S));
}
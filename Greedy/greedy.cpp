#include <iostream>
#include "greedy.h"

#define eps 1e-9

using namespace std;

int max_marginal_gain_element(bitset<N> &S, double l, double B){
    
    double max = INT_MIN;
    int g_max = -1;
    
    for(int i = 0; i < N; i++){
        if(S[i] == 1)
            continue;
        
        double m_g = marginal_gain(S, i, l, B);
        if(m_g - max > eps){
            max = m_g;
            g_max = i;
        }
    }

    return g_max;
}

pair<bitset<N>, double> greedy(double l, double B){
    bitset<N> S;
    
    S.reset();

    while((int)S.count() < K){
        int greedy_choice = max_marginal_gain_element(S, l, B);

        S[greedy_choice] = 1;
        
        printf("The value at iteration %d : %.6lf\n" , (int)S.count() , zfun(S, l, B));
        cout << "These are the elements: " << endl;

        for(int i = 0; i < N; i++) {
            if(S[i] == 1){
                cout << i << " ";
            }
        }

        cout << endl << endl;
    }


    return make_pair(S, zfun(S, l, B));
}
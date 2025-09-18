#include <iostream>
#include "greedy.h"

#define eps 1e-18

using namespace std;

int max_marginal_gain_element(bitset<N> &S, double l, double B, int which){
    
    double max = INT_MIN;
    int g_max = -1;
    
    for(int i = 0; i < N; i++){
        if(S[i] == 1)
            continue;
        
        double m_g = marginal_gain(S, i, l, B, which);
        if(m_g - max > eps){
            max = m_g;
            g_max = i;
        }
    }

    return g_max;
}

pair<bitset<N>, double> greedy(bitset<N> &S, double l, double B, int which){
    
    while((int)S.count() < K){
        int greedy_choice = max_marginal_gain_element(S, l, B, which);

        S[greedy_choice] = 1;
        
        printf("The value at iteration %d : %.6lf\n" , (int)S.count() , fun(S, l, B, which));
        cout << "These are the elements: " << endl;

        for(int i = 0; i < N; i++) {
            if(S[i] == 1){
                cout << i << " ";
            }
        }

        cout << endl << endl;
    }


    return make_pair(S, fun(S, l, B, which));
}
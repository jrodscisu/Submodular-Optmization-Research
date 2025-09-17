#include "zfun.h"

double dummy_zfun( bitset<N> &S){
    return  S.count();
}

double zfun(bitset<N> &S, double l, double B){

    int start_GreedyBreakers = R * K;
    double value = 0.0;

    for(int i = 0; i < start_GreedyBreakers; i++){
        if(S[i] == 1){
            if(S[(i/K) + start_GreedyBreakers] == 1){
                value += (l - ((l + B)/K));      
            }else{
                value += l;
            }
        }
    }

    for(int i = start_GreedyBreakers; i < N - RANDOMS; i++){
        if(S[i] == 1){
            value += l + B;
        }
    }

    for(int i = N - RANDOMS; i < N; i++){
        if(S[i] == 1){
            value += sqrt(l);
        }
    }

    return value;
}

double marginal_gain(bitset<N> S, int g, double l, double B){
    double zfunUe = zfun(S, l, B);
    
    S[g] = 1;

    double zfunUi = zfun(S, l, B);

    return zfunUi - zfunUe;
}

#include "zfun.h"

bitset<N> mask;

void generator(){
    mask.reset();
    for(int i = R * K; i < N - RANDOMS; i++){
        mask[i] = 1;
    }
}

double zfun(bitset<N> &S, double l, double B){

    int start_GreedyBreakers = R * K;
    double value = 0.0;

    bitset<N> Rs = S & mask;

    int n_Rs = Rs.count();    

    for(int i = 0; i < start_GreedyBreakers; i++){
        if(S[i] == 1){
            if(n_Rs > 0){
                value += (l - ((n_Rs) * (l + B)/K));      
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

double zfun_Non_Disjoint(bitset<N> &S, double l, double B){
    int start_GreedyBreakers = R * K;

    double value = 0.0;

    for(int i = 0; i < start_GreedyBreakers; i++){
        if(S[i] == 1){
            if(S[(i/K) + start_GreedyBreakers] == 1){
                double discount_rate = (2.0 * (double)((int)(i % K) + 1)) / (K * (K + 1)); 

                value += (l - discount_rate * ((double)R * (l + B)));
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

double fun(bitset<N> &S, double l, double B, int which){
    if(which == 0){
        return zfun(S, l, B);
    }else{
        return zfun_Non_Disjoint(S, l, B);
    }
}

double marginal_gain(bitset<N> S, int g, double l, double B, int which){
    generator();
    double zfunUe = fun(S, l, B, which);
    
    S[g] = 1;

    double zfunUi = fun(S, l, B, which);

    return zfunUi - zfunUe;
}

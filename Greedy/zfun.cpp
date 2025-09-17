#include "zfun.h"

double ALPHA = 2.0 * exp(1.0);
double Beta = exp(1.0) - 1;

double dummy_zfun(int N, int K, set<int> &S){
    return ALPHA * S.size();
}

double zfun(int N, int K, set<int> &S){
    return dummy_zfun(N, K, S);
}

double marginal_gain(int N, int K, set<int> S, int g){
    double zfunUe = zfun(N, K, S);
    
    S.insert(g);

    double zfunUi = zfun(N, K, S);

    return zfunUi - zfunUe;
}

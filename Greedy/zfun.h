#ifndef ZFUN_H
#define ZFUN_H

#include <set>
#include <math.h>

using namespace std;

double marginal_gain(int N, int K, set<int> S, int g);
double zfun(int N, int K, set<int> &S);

#endif // ZFUN_H
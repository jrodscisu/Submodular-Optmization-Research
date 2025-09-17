#ifndef ZFUN_H
#define ZFUN_H

#include <bitset>
#include <math.h>

#define K 8
#define R 2
#define RANDOMS 0
#define N (K + 1) * R + RANDOMS

using namespace std;

double marginal_gain(bitset<N> S, int g, double l, double B);
double zfun(bitset<N> &S, double l, double B);

#endif // ZFUN_H
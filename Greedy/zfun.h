#ifndef ZFUN_H
#define ZFUN_H

#include <bitset>
#include <math.h>
#include <cstdlib>
#include <ctime>
#include <vector>

#ifndef K
#define K 64
#endif

#ifndef R
#define R K / 4
#endif

#ifndef RANDOMS
#define RANDOMS 0
#endif

#ifndef N
#define N (K + 1) * R + RANDOMS
#endif

using namespace std;

double marginal_gain(bitset<N> S, int g, double l, double B, int which);
double zfun(bitset<N> &S, double l, double B);
double zfun_Non_Disjoint(bitset<N> &S, double l, double B);
double fun(bitset<N> &S, double l, double B, int which);

#endif // ZFUN_H
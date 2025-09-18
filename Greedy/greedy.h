#ifndef GREEDY_H
#define GREEDY_H
#include "zfun.h"

int max_marginal_gain_element(bitset<N> &S, double l, double B, int which);

pair<bitset<N>, double> greedy(bitset<N> &S, double l, double B, int which);

#endif
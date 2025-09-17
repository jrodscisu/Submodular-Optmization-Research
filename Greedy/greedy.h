#ifndef GREEDY_H
#define GREEDY_H
#include "zfun.h"

int max_marginal_gain_element(bitset<N> &S, double l, double B);

pair<bitset<N>, double> greedy(double l, double B);

#endif
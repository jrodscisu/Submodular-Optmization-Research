#ifndef GREEDY_H
#define GREEDY_H
#include "zfun.h"

int max_marginal_gain_element(int N, int K, set<int> &S);

pair<set<int>, int> greedy(int N, int K);

#endif
#include <iostream>
#include "greedy.h"

using namespace std;

int main() {

    double l, B;

    scanf("%lf %lf", &l, &B);

    pair<bitset<N>, double> answer = greedy(l, B);


    cout << "The returned value is " << answer.second << endl;


    cout << endl;
    return 0;
}

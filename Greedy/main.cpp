#include <iostream>
#include <string>
#include "greedy.h"

using namespace std;

int main(int argc, char * argv[]) {

    double l = stod(argv[1]), B = stod(argv[2]);
    int choice = stod(argv[3]);

    bitset<N> S;

    S.reset();
    if (stoi(argv[5]) > 0){
        int x = stoi(argv[5]); 

        if(string(argv[4]) == string("0")){
            while(x--){
                S[x] = 1;
            }
        }else{
            while(x--){
                S[x * K] = 1;
            }
        }
    }

    pair<bitset<N>, double> answer = greedy(S, l, B, choice);

    double approximation = answer.second / (K * l);

    // printf("K=%d, R=%d, Randoms=%d, l=%.6lf, B=%.6f, Disjoint=%s, Approximation=%.9f\n",
    //        K, R, RANDOMS, l, B, ((choice) ? "False" : "True"), approximation);


    printf("%d %d %d %.6lf %.6f %d %s %s %.9f\n", 
        K, R, RANDOMS, l, B, ((choice) ? 0 : 1), argv[4], argv[5] , approximation);

    return 0;
}

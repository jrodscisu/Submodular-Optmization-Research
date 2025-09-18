#include <iostream>
#include <string>
#include "greedy.h"

using namespace std;

int main(int argc, char * argv[]) {

    double l, B;
    int choice;
    scanf("%lf %lf %d", &l, &B, &choice);

    bitset<N> S;

    S.reset();
    if (argc > 1){
        for(int i = 1; i < argc; i++){
            S[stoi(string(argv[i]))] = 1; 
        }
    }

    pair<bitset<N>, double> answer = greedy(S, l, B, choice);

    double approximation = answer.second / (K * l);

    printf("%.9f\n", approximation);

    // cout << "The returned value is " << answer.second << endl;


    // cout << "These are the elements: " << endl;

    // for(int i = 0; i < N; i++) {
    //     if(answer.first[i] == 1){
    //         cout << i << " ";
    //     }
    // }



    // cout << endl << endl;
    // cout << endl;
    return 0;
}

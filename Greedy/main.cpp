#include <iostream>
#include "greedy.h"

using namespace std;

int main() {

    int N, K;

    cin >> N >> K;

    pair<set<int>, double> answer = greedy(N, K);


    cout << "The returned value is " << answer.second << endl;

    cout << "These are the elements: " << endl;

    for(auto x : answer.first) {
        // cout << x << " ";
    }
    cout << endl;
    return 0;
}

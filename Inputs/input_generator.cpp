#include <iostream>

using namespace std;

int main(int argc, char * argv[]){

    for(int i = 0; i <= 10; i++){
        cout << 10 << ' ' << i << ' ' << string(argv[1]) << endl;
    }

    return 0;
}
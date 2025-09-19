#include <iostream>

using namespace std;

int main(int argc, char * argv[]){

    for(int i = 0; i <= 10; i++){
        printf("10 %d %s %s %s\n", i, argv[1], argv[2], argv[3]);
    }

    return 0;
}
#include <iostream>
#include <fstream>

using namespace std;

int main() {
    ifstream fin("ratings.txt");
    int a, b;

    fin >> a >> b >> ws;

    cout << a << ' ' << b << endl;

    while(!fin.eof()){
        fin >> a >> b >> ws;
        cout << b << ' ' << a + 9724 << ' ' << 1.0 << endl;
    }

    return 0;
}
#include <iostream>
using namespace std;

int sum(int a, int b){
    return a + b;
}

int main() {
    int a, b, c;
    a = 3; b = 5;
    c = sum(a, b);
    cout << "c: " << c << endl;
    return 0;
}

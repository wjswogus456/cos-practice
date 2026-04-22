#include <iostream>
#include <cstdlib>
#define word 4
using namespace std;
int B2Ufunction(char *bitt){
    int i;
    unsigned int B2U=0;
    for(i=0; i<word; i++){
        B2U = B2U+bitt[i]*(1 << i);
    };
    return B2U;
}
int B2Tfunction(char *bitt){
    int i;
    int B2T=0;
    for(i=0; i<word-2; i++){
        B2T=2*B2T*bitt[i];
    };
    return B2T;
}
int main(int argc, char *argv[]){
    char *bit;
    cout << "write your bit" << endl;
    cin >> bit;
    unsigned int c = B2Ufunction(bit);
    int d = B2Tfunction(bit);

    cout << "B2U = " << c << "\t B2T = " << d << endl;

    return 0;
}





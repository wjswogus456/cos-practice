#include <iostream>
#define BUFLEN 10
using namespace std;
int main(int argc, char *argv[]){
    int arr[BUFLEN];
    int *p;
    int a, i;

    for (i=0; i<BUFLEN; i++)
        arr[i] = 10-i;

    a = 5;
    p = &a;

    cout << "a = " << a << endl;
    cout << "p = " << p << endl;
    cout << "&a = " << &a << endl;
    cout << "*p = " << *p << endl;
    cout << "arr = " << arr << endl;

    return 0;
}

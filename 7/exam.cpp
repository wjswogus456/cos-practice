#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace std;

int main(int argc, char *argv[]){
    const char *str = "character";
    int num=0;
    int i=0;
    while(str[i] != 00){
        num = num+1;
        i = i+1;
    }
    cout << "the number is = " << num << endl;

}
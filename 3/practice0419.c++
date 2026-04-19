#include <iostream>
#include <cstdlib>
using namespace std;

int main (int argc, char *argv[]){
    
    if (argc > 3){
        cout << "You must enter only 2 numbers" << endl;
        return 0;
    }else if(argc <3){
        cout << "you must enter at least 2 numbers" << endl;
        return 0;
    }else if(argc == 3){
        int x = atoi(argv[1]);
        int y = atoi(argv[2]);
        int c = x % y;
        cout << "x mod y = " << c << endl;
    }
    return 0;

}
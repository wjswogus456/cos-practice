#include <iostream>
#include <cstdlib>
using namespace std;
int i;
int main(int argc, char *argv[]){
    int a[11];
    for(i=0; i<=10; i++){
        a[i]=10-i;
     cout << "arr[" << i << "] = " << a[i] << "\n"; 
    }
    
    int p;
    p = atoi(argv[1]);
    switch (p) {
        case 30:
        cout << "your argument is = " << p << endl;

        break;
        default:
        cout << "else" << endl;
        break;
    }
    return 0;
}

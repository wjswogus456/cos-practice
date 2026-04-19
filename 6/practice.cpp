#include <iostream>
#include <cstdlib>
using namespace std;

int check_validity(int x) {
    if (x >= 1 && x <= 10){
        return 1;       
    }else{
        return 0;
    }
}

int main(int argc, char *argv[])
{
    int i, x;
    int *numbers;
    cout << "please write the number = " << endl;
    cin >> x;
    while(check_validity(x) !=1){
        cout << "plz write the correct number" << endl;
        cin >> x;
    }

    if (check_validity(x) == 1){
        int *space;
        space = (int *)malloc(sizeof(int) * x);
        numbers = space;
        for(i=0; i<x; i++){
            int n;
            cout << "please writne the numbers[]" << endl;
            cin >> n ;
            space[i] = n;
                    }
        for(i=0; i<x; i++){
            cout << "numbers[" << i << "] = " << space[i] << endl;
        }
    }
    
    return 0;
}

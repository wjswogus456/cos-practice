#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;
int check_validity(int a, int b){
    if(a<b){
        return 1;
    }else{
        return 0;
    }

}
int sum_up(int a, int b){
    int sum = 0;
    int i;
    for(i=a; i<=b; i++){
        sum = sum+i;
    }
    return sum;
}
int main(int argc, char *argv[]){
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    if (check_validity(a,b) == 1){
        
    cout << "sum = " << sum_up(a,b) << endl;

    }else{
        return 0;
    }
    return 0;
}
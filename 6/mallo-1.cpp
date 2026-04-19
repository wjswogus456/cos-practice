#include <iostream>
#include <cstdlib>
using namespace std;
int main(int argc, char *argv[]){
    int *p;
    int i;

    p = (int *)malloc(sizeof(int) * 10);
    for (i=0; i<10; i++)
        p[i] = 10 - i;
    for (i=0; i<10; i++)
        cout << "p[" << i << "]= " << p[i] << endl;
    free(p);

    return 0;
}

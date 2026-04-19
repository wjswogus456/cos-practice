#include <iostream>
#include <cstdlib>
using namespace std;

typedef struct human_st {
    int cal ;
    int foodnumber ;
    const char *food;
    void (* printfunction)(struct human_st *);
} human_t;

void printfood(human_t *a){

    cout << "food calories = " << a->cal << "\tfoodnumber = " << a->foodnumber << "\tfood name = " << a->food << endl;

};

int main(int argc, char *argv[]){

    human_t *fd;
    fd = (human_t *)malloc(sizeof(human_t));
    
    fd->cal = 3256;
    fd->foodnumber = 1;
    fd->food = "chicken";
    fd->printfunction = printfood;
    
    fd->printfunction(fd);
    return 0;
}
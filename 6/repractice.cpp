#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace std;

struct foodgroup {
    int number;
    const char *name;
    void (*print)(struct foodgroup *);
};

void printfood(struct foodgroup *fd){
    cout << "number = " << fd->number << "\nname = " << fd->name << endl;
}

int main(int arbc, char *argv[]){
    struct foodgroup *fd;
    fd = (struct foodgroup *)malloc(sizeof(struct foodgroup));
    fd->number = 3;
    fd->name = "chamber";
    fd->print = printfood;
    fd->print(fd);
    return 0;
}
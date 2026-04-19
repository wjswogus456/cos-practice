#include <iostream>
#include <cstdlib>
#include <cstring>

typedef struct food{
    int number;
    const char *name;
    void (*foodfunction)(struct food *);
} food_t;

void function(food_t *fd){
    한번 연습해보시오.
}

int main (int argc, char *argv[]){

    food_t *fd;
    fd = (food_t *)malloc(sizeof(food_t));

    fd->number = 1;
    fd->name = "chicken";
    fd->foodfunction = function;

    fd->foodfunction(fd);

}
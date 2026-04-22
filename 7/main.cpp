#include <iostream>
#include "human.h"
using namespace std;

int main() {
    Human *hw;
    hw = new Human("Hyunwoo Lee");
    hw->print();
    hw->setAge(37);
    hw->print();
}

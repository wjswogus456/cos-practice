#include <iostream>
#include <cstdlib>
#include <cstring>
using namespace	std;

int main(int argc, char *argv[]){
	int x = atoi(argv[1]);
	int y = atoi(argv[2]);
	int i;
	int sum;
		if (x<y){
			for(i=x; i<=y; i++){
			sum = sum+i;
			}
			cout << "sum = " << sum << endl;
		}else{
		cout << "sry you must write the correct numbers" << endl;
	}
	return 0;
}
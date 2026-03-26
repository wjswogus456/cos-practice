#include <iostream>
#define BUFLEN 10
using namespace std;
int main(int argc, char *argv[]){
	int arr[BUFLEN];
	int p;
	int &a;

	for (i=0; i<BUFLEN; i++)
		arr[i] = 10-i;


		a = 5;
		p = &a;

		cout << "a = " << a << endl;
		cout << "p = " << p << endl;

		p=10;

		cout << "a = " << a << endl;
		cout << "p = " << p << endl;
		return 0;
}


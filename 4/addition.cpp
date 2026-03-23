#include <iostream>
#include <cstdlib>
using namespace std;
int uadd_ok(unsigned a, unsigned b)
{
	int sum = a + b;

	if(sum < a)
		return 0;
	return 1;
}

int tadd_ok(int a, int b)
{
	int sum = a + b;
	if (a > 0 && b > 0 && sum < 0)
		return 0;
	else if ( a < 0 && b < 0 && sum > 0)
		return 0;
	return 1;
}

int main(int argc, char *argv[])
{
	int a, b;
	unsigned x, y;

	if (argc!=3)
	{
		cout<< "You should insert two numbers in a hexadecimal way" << endl;
		return 0;
	}
	a = strtol(argv[1], NULL, 16);
	b = strtol(argv[2], NULL, 16);
	x = (unsigned) a;
	y = (unsigned) b;

	cout << "Unsigned Addition ===" << endl;
	if (uadd_ok(x,y))
		cout << " No overflow" <<endl;
	else
		cout << "Overflow" << endl;

	cout << "Signed Addition ===" << endl;
	if (tadd_ok(a,b))
		cout << "No overflow" << endl;
	else
		cout << "Overflow" << endl;
	return 0;
}

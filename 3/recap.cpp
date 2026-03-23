#include <iostream>
using namespace std;
int main(int argc, char *argv[])
{
	int a, b, sum, num;
	if ( argc != 3)
	{
		cerr<<"Error! You must input two nubmers!" <<endl;
		return 1;
	}
	a = atoi(argv[1]);
	b = atoi(argv[2]);

	if (a >=b)
	{
		cerr << "Error! The second argument must be bigger than the first argument" <<endl;
		return 1;
	}

	num = a ;
	sum = 0;
	while (num <=b)
	{
		sum += num;
		num +=1;
	}

	cout << "Sum: " << sum << endl;

	return 0;
}


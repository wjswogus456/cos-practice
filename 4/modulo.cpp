#include <iostream>

#include <cstdlib>
using namespace std;
int modulo(int a, int b)
{
	int ret;
	ret = a % b;

	if (ret < 0)
		ret += b;

	return ret;
}

int main(int argc, char *argv[])
{
	int a,b,c;
	if (argc != 3) {
		cout << "Please insert two numbers" << endl;
		return 0;
	}

	a=atoi(argv[1]);
	b=atoi(argv[2]);
	//c=a%b;
	c=modulo(a,b);
	cout << a << "mod" << b << " = " << c << endl;

	return 0;
}


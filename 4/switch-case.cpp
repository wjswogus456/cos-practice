#include <iostream>
using namespace std;
int main(int argc, char *argv[])
{
	int x{};
	cout << "Enter a number: ";
	cin >> x;
	switch (x) {
		case 1:
		cout << "You entered 1" << endl;
		break;
		case 2:
		cout << "You entered 2" << endl;
		break;
		default:
		cout << "You entered unknown number" << endl;
	}
	return 0;
}

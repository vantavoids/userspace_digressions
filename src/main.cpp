#include <iostream>
#include <unistd.h>

int main()
{
	std::cout << "gressions :D" << "\n";
loop:
	pause(); //be nice to mr. cpu
	goto loop;
}

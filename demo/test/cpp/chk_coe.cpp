
#include "iostream"
#include "stdio.h"
#include "nodes"
#include "coear"
#include <new>
extern "C" stduint _MALCOUNT = 0;
extern "C" stduint _MALLIMIT = 0x1000;
extern "C" stduint call_state = 0;

extern "C" {
	void erro(char* str) {}
}

void body() {
	uni::Coe co1("+1","+1");
	uni::Coe co2(2);
	uni::Coe co3("-3");
	uni::Coe co4 = co3;
	double flt = co4.GetReal();
	std::cout << co2 << " is +2 while -3 is " << flt << std::endl;
	std::cout << "+1 is " << co1.ToString() << std::endl;
	++co3; co3++;
	std::cout << "-1 is " << co3.ToString() << std::endl;
	--co3; co3--;
	std::cout << "-3 is " << co3.ToString() << std::endl;
	std::cout << "-3 + 2 + 1 is " << 0 + (+co2) + co3 + 1 << std::endl;
	std::cout << "+3 is " << -co3 << " is " << 0 - co3 << std::endl;
	std::cout << "1 + 3 is " << co1 - co3 - 0 << std::endl;
	std::cout << "+2 * -3 / 4 is " << 1 * co2 * co3 * 0.5 / 2 << std::endl;
	std::cout << "+2 / -3 is " << co2 / co3 << std::endl;
	std::cout << "+12 / -3 is " << 12 / co3 << std::endl;
	std::cout << "pi is " << uni::Coe::pi() << std::endl;
	std::cout << "e is " << uni::Coe::e() << std::endl;
	std::cout << "sin(pi) is " << uni::Coe::pi().Sin() << std::endl;
}

int main()
{
	body();
	if (_MALCOUNT)
		std::cout << "MEMLEAK " << _MALCOUNT << std::endl;
}

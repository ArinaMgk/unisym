// 20240311 g++ chk_strpool.cpp ../../../lib/cpp/strpool.cpp
#define _DBEUG
#include "../../../inc/cpp/strpool"
#include "iostream"
extern "C" int _MALCOUNT = 0;
int main() {
	uni::Strpool sp;
	// sp.~Strpool();
	std::cout << sp.AllocString("Hello, world!") << std::endl;
	sp.~Strpool();
	std::cout << _MALCOUNT << std::endl;
}

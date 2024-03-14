// 20240310
#include "../../../inc/cpp/stack"
#include "iostream"

int main() {
	uni::Stack<unsigned> sb(5);
	std::cout << sb.isEmpty() << sb.isFull() << std::endl;
	sb.push(1);
	std::cout << sb.isEmpty() << sb.isFull() << std::endl;
	sb.push(2);
	sb.push(3);
	sb.push(4);
	sb.push(5);
	std::cout << sb.isEmpty() << sb.isFull() << std::endl;
	std::cout << (unsigned int)sb.pop() << std::endl;
	std::cout << sb.isEmpty() << sb.isFull() << std::endl;
}

/* OUTPUT
10
00
01
5
00
*/
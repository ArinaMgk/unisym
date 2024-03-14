// 20240312
// g++ chk_nodes.cpp ../../../lib/cpp/nodes/*.cpp -g
#define _DEBUG
#include "../../../inc/cpp/nodes"
#include "iostream"
extern "C" stduint _MALCOUNT = 0;
int main() {
	uni::NodeChain nc(false);
	nc.Append((void*)0x00000001);
	nc.Append((void*)0x00000002);
	std::cout << nc.Count() << std::endl;
	nc.~NodeChain();

	uni::InodeChain ic(false);
	ic.Append((void*)0x00000003, 0, 0, 0);
	ic.Append((void*)0x00000004, 0, 0, 0);
	std::cout << ic.Count() << std::endl;
	ic.~InodeChain();

	uni::DnodeChain* dc = new uni::DnodeChain(false);
	dc->Append((void*)0x00000005, 0);
	dc->Append((void*)0x00000006, 0);
	dc->Append((void*)0x00000007, 0);
	std::cout << dc->Count() << std::endl;
	dc->~DnodeChain();

	uni::TnodeChain tc(false);// not for `.identifier`
	tc.Append("hello", 3, 0, 0, 0);
	tc.Append("world", -1, 0, 0, 0);
	std::cout << (char*)tc.Root()->offs << std::endl;
	std::cout << (char*)tc.Root()->next->offs << std::endl;
	std::cout << tc.Count() << std::endl;
	tc.~TnodeChain();

	uni::NnodeChain nc2(false);
	nc2.Append((void*)0x0000000A, 0, 0, 0);
	nc2.Append((void*)0x0000000B, 0, 0, 0);
	std::cout << nc2.Count() << std::endl;
	nc2.~NnodeChain();

	// ---- ---- ---- ----
	if (_MALCOUNT) std::cout << "MEMLEAK " << _MALCOUNT << std::endl;
}

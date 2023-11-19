#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include"../ustring.h"
//This program is to check (others are to do):
// - dnode 
// - node
// RFQ28 RFR01
// gcc "(d)node.c" ../source/*.c -m64 -D_dbg -o dnode.exe
size_t malc_count;

void NodesPrint(Node* first)
{
	Node* next;
	while (next = first)
		if ((first = next->next, next->addr))
			puts(next->addr);
}

void main()
{
	printf("\n [ %llx]\n", malc_count);
	Dnode* n = 0;
	n = DnodeCreate(n, (char*)10, 1);
	DnodeCreate(n, (char*)20, 5);
	DnodeCreate(n, (char*)15, 6);
	DnodeCreate(n, (char*)15, 6);
	DnodeCreate(n, (char*)5, 6);
	DnodeCreate(n, (char*)0, 10);
	n = DnodeRewind(n);
	do printf(" %lld ", n->addr); while ((n->next) && (n = n->next));
	n = DnodeRewind(n);
	DnodesRelease(n);
	printf("\n [ %llx]\n", malc_count);

	Node* nd = 0;
	nd = NodeCreate(nd, "1");
	NodeCreate(nd, "2");
	NodeCreate(nd, "3");
	NodeInsert(nd->next, "2.5");
	NodesPrint(nd);
	NodesRelease(nd);

	printf("\n [ %llx]\n", malc_count);
}

// Heap String Dchain, @dosconio at 20250115
Dchain chn(DnodeHeapFreeSimple);
chn.AppendHeapstr((char*)"a");
chn.AppendHeapstr((char*)"b");
fori(i, chn) {
	forhead(i, Dnode);
	Console.OutFormat("%s\n", i.addr);
}
// Readonly String Dchain, @dosconio at 20250115
Dchain chn;
chn.Append("a");
chn.Append("b");
fori(i, chn) {
	forhead(i, Dnode);
	Console.OutFormat("%s\n", i.addr);
}
//
/*[Host Output]
* a
* b
*/

// ASCII CPP-ISO2x TAB4 CRLF
// Docutitle: Brancher Main Program
// Codifiers: @dosconio: 20250211- 
// Attribute: Host(Linux)
// Copyright: Apache License 2.0

/*
[parameter]
exist:
	prefix ipt-: Intel PT  Linux-perf script
	prefix lbr-: Intel LBR Linux-perf script
nonexist:
	list hardware branch predictor

[workbench] (all x64)
	compiler: CodeStitcher on Ubuntu 18 LTS
	sampiler: Intel PT on xeon-e5 Ubuntu 16.04.1
	building: Ubuntu Jammy LTS
*/

// 20250513
//          - auto detect user area below 0x40000000
//          - restructure and adapt for c++

#include "brancher.hpp"
#include <stdio.h>
#include "c/mnode.h"

extern "C" int system (const char *__command);

use crate uni;

Dchain* want_trace;
Dchain* linksym_lists;

String* file_trace;// logfile name
String* file_symbols;// symbolfile name


// [FILTER]
bool autofuncs_list = true;//{TODO} specified functions

_Comment("[TODO] Multidoc Support.");


int get_basicblock_id(const char* bb_iden, const char* bb_name) {
	int id = -1;
	if (StrLength(bb_iden) <= StrLength(bb_name)) {
		return id;
	}
	bb_iden += StrLength(bb_name) + 1;// main*1-16-{"4":{"type":"T"},"2":{"type":"F"}}*[]
	sscanf(bb_iden, "%d", &id);
	return id;
}

// for Mnode
static int my_compare(pureptr_t a, pureptr_t b) {
	return StrCompare((const char*)a, (const char*)b);
}
static void my_free(pureptr_t c) {
	Letvar(cc, Dnode*, c);
	mfree(cc->addr);
	mfree(cc->type);
}

void brancher() {
	//ploginfo("Brancher 2.0");
	String crtfunc = "";

	static int last_idx = -1;// avoid calling
	static int last_bid = -1;

	HostFile file (file_trace->reference());
	if (!file) {
		plogerro("log-file %s not exist.", file_trace->reference());
		return;
	}

	Mchain mc1;
	mc1.func_comp = my_compare;
	mc1.refChain().func_free = DnodeHeapFreeSimple;

	char* buffer = (char*)malc(2048);
	while (file.FetchLine(buffer, 2048)) {
		//            49b4a2 =>           49b4ad
		if (buffer[0] != ' ') continue;
		char* p = buffer;
		while (*p == ' ') p++;
		uint64 src = atohex(p);
		p = (char*)StrIndexString(p, "=>");
		if (!p) continue;
		p += 2;
		while (*p == ' ') p++;
		uint64 dst = atohex(p);

		// Console.OutFormat("%s%[64H] -> %[64H]\n\n", buffer, src, dst);

		if (crtfunc.length() && src) {
			stduint prev_start = mc1[(pureptr_t)crtfunc.reference()].lens;
			stduint last_start = prev_start;
			stduint last_addr = Linksyms::Index(*linksym_lists, crtfunc.reference(), last_start);
			while (last_addr < src) {
				Console.OutFormat("%s*%[u]\n", crtfunc.reference(), last_start);
				last_start++;
				last_addr = Linksyms::Index(*linksym_lists, crtfunc.reference(), last_start);
				if (last_addr == 0) break;
			}
			mc1[(pureptr_t)crtfunc.reference()].lens = last_start - 1;
			// ploginfo("OwO %s(%[u]~%[u])", crtfunc.reference(), prev_start, last_start - 1);
		}

		if (dst) {
			bool found = false;
			stduint last_off = 0;
			for (auto var = linksym_lists->Root(); var; var = var->next) {
				Linksym* linksym = (Linksym*)var->offs;
				if (dst == linksym->offs) {
					String tmp = linksym->name;
					char* tmp2 = NULL;
					stduint now_start = 0;
					if (tmp2 = (char*)tmp.Index('*')) {
						now_start = atoins(tmp2 + 1);
						*tmp2 = '\0';
						tmp.Refresh();
					}
					crtfunc = tmp;
					Letvar(res, Mnode*, mc1.refChain().LocateNode((pureptr_t)crtfunc.reference(), my_compare));
					if (!res) {
						mc1.Map(StrHeap(crtfunc.reference()), nullptr);
					}
					mc1[(pureptr_t)crtfunc.reference()].lens = now_start;
					found = true;
					// ploginfo("Enter Block: (%s) of func (%s)(%[u])", linksym->name, crtfunc.reference(), now_start);
					break;
				}
				else if (last_off && dst < linksym->offs && dst > last_off) {
					stduint mustlen = StrIndexChar(linksym->name, '*') - linksym->name;
					if (!StrCompareN(linksym->name,
						((Linksym*)(var->left->offs))->name,
						mustlen
					)) {
						String tmp = ((Linksym*)(var->left->offs))->name;
						char* tmp2 = NULL;
						stduint now_start = 0;
						if (tmp2 = (char*)tmp.Index('*')) {
							now_start = atoins(tmp2 + 1);
							*tmp2 = '\0';
							tmp.Refresh();
						}
						crtfunc = tmp;
						Letvar(res, Mnode*, mc1.refChain().LocateNode((pureptr_t)crtfunc.reference(), my_compare));
						if (!res) {
							mc1.Map(StrHeap(crtfunc.reference()), nullptr);
						}
						mc1[(pureptr_t)crtfunc.reference()].lens = now_start;
						found = true;
						// ploginfo("Enter Block: (%s) of func (%s)(%[u])", ((Linksym*)(var->left->offs))->name, crtfunc.reference(), now_start);
						break;
					}
				}
				last_off = linksym->offs;
			}
			if (!found) crtfunc = "";
		}
		else crtfunc = "";

	}


	puts("");
	mfree(buffer);
}


int main(int argc, char** argv) {
	enum class crtmode_e {
		logfile,// output of `perf script` aka `brancher record`
		symfile,// output of `nm ./a.out`
		function
	};
	crtmode_e crtmode = crtmode_e::function;
	_logstyle = _LOG_STYLE_GCC;
	//
	want_trace = new Dchain();
	want_trace->func_free = DnodeHeapFreeSimple;
	linksym_lists = new Dchain();
	linksym_lists->func_free = BrancherFree_Linksym;
	// Brancher Default Filename inherit from v1.0
	file_trace = new String("perf.txt");
	file_symbols = new String("perf.name");

	if (argc < 2) {
		plogwarn("Usage for perf script:\n\t%s filter -f <logfile> -l <symfile> (<funcs...>)\n\t%s", argv[0],
			"# If <funcs> not specified, all user functions will be considered.");
		plogwarn("Usage for perf branch:\n\tsudo %s record <filepath>", argv[0]);
		goto endo;
	}


	if (!StrCompare(argv[1], "record") && argc == 3)// brancher record test.exe
	{
		rostr out_perf = "perf.data";
		ploginfo("Please use SUDO mode running this. like sudo ./brancher record ./test");
		int status = system(String::newFormat
			("perf record -e intel_pt//u -o %s -- %s", out_perf, argv[2]
			).reference());
		if (status == 0)
		{
			ploginfo(String::newFormat("Some information stored in %s.", out_perf).reference());
			rostr opt = "ip,addr";// "addr,sym"
			system(String::newFormat("perf script -F %s -i %s > perf.txt", opt, out_perf).reference());
		}
		else {
			plogerro("Bad %d!", status);
		}
		goto endo;
	}
	if (StrCompare(argv[1], "filter")) {
		plogerro("Bad Command: %s", argv[1]);
		goto endo;
	}

	//{} below are filter

	argv += 2; argc -= 2;
	for0(i, argc) {
		if (!StrCompare(argv[i], "-f")) {
			crtmode = crtmode_e::logfile;
			continue;
		}
		if (!StrCompare(argv[i], "-l")) {
			crtmode = crtmode_e::symfile;
			continue;
		}
		//
		if (crtmode == crtmode_e::function) {
			want_trace->AppendHeapstr(argv[i])->lens = -2;
			autofuncs_list = false;
		}
		else if (crtmode == crtmode_e::logfile) {
		    *file_trace = argv[i];
			crtmode = crtmode_e::function;
		}
		else if (crtmode == crtmode_e::symfile) {
			*file_symbols = argv[i];
			crtmode = crtmode_e::function;
		}
	}

	if (0) ploginfo("we will read from %s and filter %d functions", 
		file_trace->reference(),
		want_trace->Count());

	{
		//{TEMP} assume this PT script
		HostFile symf(file_symbols->reference());
		if (!bool(symf)) {
			plogerro("sym-file %s not exist.", file_symbols->reference());
			return -1;
		}
		Linksyms::StructSymbols(*linksym_lists, symf);
		//if (want_trace->Count()) brancher();//{TODO} if count zero, print all blocks
		brancher();
	}
endo:
	delete file_symbols;
	delete file_trace;
	delete linksym_lists;
	delete want_trace;
	if (malc_count) plogerro("exallot %d times", malc_count);
	return malc_count;
}

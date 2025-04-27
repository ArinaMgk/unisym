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
	sampiler: Intel PT on neu-xeon-e5 Ubuntu 16.04.1
	building: Ubuntu Jammy LTS
*/

// 20250303 fix calling function jumpings

#define _STYLE_RUST
#include <c/consio.h>
#include <stdio.h>
#include <unistd.h>

extern "C" int system (const char *__command);

use crate uni;

Dchain want_trace;
String file_trace;// logfile name

_Comment("[TODO] Multidoc Support.");
_Comment("[TODO] FUNC*0 maybe not exist.");


int get_basicblock_id(const char* bb_iden, const char* bb_name) {
	int id = -1;
	if (StrLength(bb_iden) <= StrLength(bb_name)) {
		return id;
	}
	bb_iden += StrLength(bb_name) + 1;// main*1-16-{"4":{"type":"T"},"2":{"type":"F"}}*[]
	sscanf(bb_iden, "%d", &id);
	return id;
}

bool exist_intel_pt() {
    return (access("/sys/bus/event_source/devices/intel_pt", F_OK) != -1);
}

void hard_list() {
	if (exist_intel_pt())
		ploginfo("Intel PT OK."); 
	else 
		plogerro("Intel PT not exist.");

}

void brancher() {
	ploginfo("Brancher 1.0");

	static int last_idx = -1;// avoid calling
	static int last_bid = -1;

	FILE* file = fopen(file_trace.reference(), "r");
	if (!file) {
		plogerro("file not exist.");
		return;
	}
	char* buffer = (char*) malc(2048);

	while (fgets(buffer, 2048, file) != NULL) {
		//branches:u:      7f4b164c06f3 do_lookup_x (/lib/x86_64-linux-gnu/ld-2.27.so)
		//=>     7f4b164c0706 do_lookup_x (/lib/x86_64-linux-gnu/ld-2.27.so)
		rostr flg1 = "branches:u:";
		char* p1 = (char*) StrIndexString(buffer, flg1);
		char* p2 = (char*) StrIndexString(buffer, "=>");
		if (!p1 || !p2) continue;
		p1 += StrLength(flg1);
		*p2 = 0;
		p2 += 2;
		p2[StrLength(p2) - 1] = 0;// remove \n
		// remove prefix spaces
		StrDeprefixSpaces(p1);
		StrDeprefixSpaces(p2);
		// then remove a UID
		p1 = (char*) StrIndexChar(p1, ' ') + 1;
		p2 = (char*) StrIndexChar(p2, ' ') + 1;
		// skip unknown related
		if (StrIndexString(p1, "[unknown] ([unknown])") || StrIndexString(p2, "[unknown] ([unknown])")) continue;
		// _TEMP skip .so
		if (StrIndexString(p1, ".so)") && StrIndexString(p2, ".so)")) continue;
		// remove parens
		*(char*)StrIndexChar(p1, ' ') = 0;
		*(char*)StrIndexChar(p2, ' ') = 0;
		// _TEMP skip if no need
		bool good1 = false;
		bool good2 = false;
		unsigned idx1, idx2;
		for0 (i, want_trace.Count()) {
			Dnode* nod = want_trace[i];
			stduint cmplen = StrLength(nod->addr);
			if (!good1 && !StrCompareN(p1, nod->addr, cmplen) &&
				(p1[cmplen] == '\0' || p1[cmplen] == '*'))
				good1 = true, idx1 = i;
			if (!good2 && !StrCompareN(p2, nod->addr, cmplen) &&
				(p2[cmplen] == '\0' || p2[cmplen] == '*'))
				good2 = true, idx2 = i;
			if (good1 && good2) break;
		}
		if (!good1 && !good2) continue;
		// jump (Bottom->Top) from "main*5" to "main*2", we count 2,3,4,5
		//     static int main_last = -2;// -1 for main, 0 for main*0-...
		
		// ("--- (%s, %s)\n", p1, p2);
		
		// If function leave (enter), set lens -2. ???->known-head
		if (!good1 && good2) {
			Dnode* nod = want_trace[idx2];
			if (!StrCompare(p2, nod->addr)) {
				nod->lens = -2;
			}
			auto p2id = get_basicblock_id(p2, want_trace[idx2]->addr);
			if (last_idx != idx2 || (last_bid != p2id))
				outsfmt(p2id >= 0 ? "%s*%d\n" : "%s\n", want_trace[idx2]->addr, p2id);
			last_idx = idx2, last_bid = p2id;
		}
		else if (good1) {
			// ("(%s, %s)\n", p1, p2);
			int crt_id = get_basicblock_id(p1, want_trace[idx1]->addr);
			if (crt_id - (int)want_trace[idx1]->lens) {
				// ("pass %d ids\n", crt_id - main_last);
				for0(i, crt_id - (int)want_trace[idx1]->lens) {
					int p1id = want_trace[idx1]->lens + i + 1;
					if (last_idx != idx1 || (last_bid != p1id))
						outsfmt("%s*%d\n", want_trace[idx1]->addr, p1id);
					last_idx = idx1, last_bid = p1id;
				}
				if (good2) {
					auto p2id = get_basicblock_id(p2, want_trace[idx2]->addr);
					if (last_idx != idx2 || (last_bid != p2id))
						outsfmt("%s*%d\n", want_trace[idx2]->addr, p2id);
					last_idx = idx2, last_bid = p2id;
				}
			}
			else {
				if (good2 && get_basicblock_id(p1, want_trace[idx1]->addr) == (int)want_trace[idx1]->lens) {
					auto p2id = get_basicblock_id(p2, want_trace[idx2]->addr);
					if (last_idx != idx2 || (last_bid != p2id))
						outsfmt("%s*%d\n", want_trace[idx2]->addr, p2id);
					last_idx = idx2, last_bid = p2id;
				}
				// else (">>> (%s, %s)\n", p1, p2);
			}
		}
		if (good2) want_trace[idx2]->lens = get_basicblock_id(p2, want_trace[idx2]->addr);

		
		

	}
	puts("");
	mfree(buffer);
	fclose(file);

	
	
}
int main(int argc, char** argv) {
	enum class crtmode_e {
		logfile, function
	};
	crtmode_e crtmode = crtmode_e::function;
	if (argc < 2) {
		plogerro("Usage for perf script:\n\t%s filter -f <logfile> <funcs...>", argv[0]);
		plogerro("Usage for perf branch:\n\tsudo %s record <filepath>", argv[0]);
		goto endo;
	}

	want_trace.func_free = DnodeHeapFreeSimple;

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
			system("perf script -i perf.data > perf.txt");
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
		if (crtmode == crtmode_e::function) {
			want_trace.Append((char*) argv[i])->lens = -2;
		}
		else if (crtmode == crtmode_e::logfile) {
		    file_trace = argv[i];
			crtmode = crtmode_e::function;
		}
	}

	ploginfo("we will read from %s and filter %d functions", 
		file_trace.reference(),
		want_trace.Count());

	//{TEMP} assume this PT script

	if (want_trace.Count()) brancher();//{TODO} if count zero, print all blocks
endo:
	want_trace.~Dchain();
	return malc_count;
}

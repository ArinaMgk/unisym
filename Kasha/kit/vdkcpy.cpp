// GBK 20220507 ArinaKit::VDiskSectorWriter LinearWriter
// E.g.> vdkcpy boot.bin vhd.vhd 0
# include <stdio.h>
# include <iostream>
# include <fstream>
# include <cstdlib>

using namespace std;

int main(int argc, char** args)
{
	/*
	 * arg2 source =args[1]
	 * arg3 dest
	 * arg4 the numof sector to start na dest
	 */
	//read
	if (argc < 4) return 1;
	unsigned char tmpc=0xFF;
	std::ifstream ifs(args[1], ios_base::in|ios_base::binary);
	//	RER07->这里错用了||ios_base::in就会替换换行的CRLF呢。
	ifs.seekg(0, std::ios::end);
	int sourcelen = ifs.tellg();
	cout<< sourcelen <<" bytes yo source."<<endl;
	ifs.seekg(0, std::ios::beg);
	char* tmp = (char*)malloc(sourcelen+1);
	ifs.read(tmp, sourcelen);
	ifs.close();

	char* h;
	std::ofstream ofs(args[2], ios_base::out | ios_base::binary |ios_base::in);
	ofs.seekp(512 * strtoul(args[3], &h, 10), ios::beg);
	//for (int y = 0x1000; y >= 0;y--)
	for (int i = 0; i < sourcelen; i++)
	{
		ofs << (unsigned char)tmp[i];
		// printf("[%02X]", (unsigned char)tmp[i]);
	}

	ofs.close();

	//printf("\n%d\n%s\n%s\n%s\n%d\n", argc, args[0], args[1], args[2], strtoul(args[3], &h, 10)); endo:getchar(); goto endo;
}


//_read:
//	
//	ifs.read(&tmpc,1);
//	//这里直接用ifs换行标志不能读,为什么还总是替换
//	if (ifs.eof()) goto _read_end;
//	printf("%02X", tmpc);
//	goto _read;
//_read_end:
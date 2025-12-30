
#rem X:/ is the unisym/
#set PATH=C:\DJGPP\BIN;C:\bin;%PATH%
#set DJGPP=C:\DJGPP\DJGPP.ENV

build: data # for linux64 [no use O3]
	cc asm/data/*.c asm/about.c asm/aasm*.cpp $(ulibpath)/cpp/fileformat/execute/*.cpp -I$(uincpath) -L$(ubinpath) -ll64d -lstdc++ -D_DEBUG -D_Linux64 -o $(ubinpath)/AMD64/Lin64/aasm --static -w
	cc asm/data/_*.c asm/data/insnsb.c asm/data/insnsd.c asm/data/regs.c asm/data/regdis.c asm/about.c asm/dasm*.cpp asm/aasm-lib.cpp -I$(uincpath) -L$(ubinpath) -ll64d -lstdc++ -D_DEBUG -D_Linux64 -DNO_STDSCAN -o $(ubinpath)/AMD64/Lin64/dasm --static -w

#{TEMP}
build-win64:
	cc asm/data/*.c asm/about.c asm/aasm*.cpp $(ulibpath)/cpp/fileformat/execute/*.cpp -I$(uincpath) -L$(ubinpath) -lw64d -lstdc++ -D_DEBUG -D_Win64 -o $(ubinpath)/AMD64/Win64/aasm --static -w
	cc asm/data/_*.c asm/data/insnsb.c asm/data/insnsd.c asm/data/regs.c asm/data/regdis.c asm/about.c asm/dasm*.cpp asm/aasm-lib.cpp -I$(uincpath) -L$(ubinpath) -lw64d -lstdc++ -D_DEBUG -D_Win64 -DNO_STDSCAN -o $(ubinpath)/AMD64/Win64/dasm --static -w

data: asm/data/_crc64_tab.c
	cd asm/todata && perl make-ins.pl
	cd asm/todata && perl pptok.pl        h  pptok.dat ../data/pptok.h
	cd asm/todata && perl pptok.pl        c  pptok.dat ../data/pptok.c
	cd asm/todata && perl pptok.pl        ph pptok.dat ./perllib/pptok.ph
	cd asm/todata && perl make-macros.pl ../data/standard.mac ../data/version.mac ../data/altreg.mac  ../data/smartalign.mac ../data/outfmt/outaout.mac ../data/outfmt/outas86.mac ../data/outfmt/outbin.mac ../data/outfmt/outcoff.mac ../data/outfmt/outelf.mac ../data/outfmt/outmacho.mac ../data/outfmt/outobj.mac ../data/outfmt/outrdf.mac ../data/outfmt/outrdf2.mac
	cd asm/todata && perl make-tokhash.pl c  make-ins.dat make-reg.dat tokens.dat > ../data/tokhash.c
	cd asm/todata && perl make-tokhash.pl h  make-ins.dat make-reg.dat tokens.dat > ../data/tokens.h
	cd asm/todata && perl make-reg.pl     h  make-reg.dat> ../data/regs.h
	cd asm/todata && perl make-reg.pl     c  make-reg.dat> ../data/regs.c
	cd asm/todata && perl make-reg.pl     fc make-reg.dat> ../data/regflags.c
	cd asm/todata && perl make-reg.pl     vc make-reg.dat> ../data/regvals.c
	cd asm/todata && perl make-reg.pl     dc make-reg.dat> ../data/regdis.c
	cd asm/todata && perl make-reg.pl     dh make-reg.dat> ../data/regdis.h


asm/data/_crc64_tab.c: asm/todata/_crc64_tab.to.c
	gcc -o asm/data/a.exe $<
	cd asm/data/ && ./a.exe
	rm asm/data/a.exe

clean:
	rm ./data/_crc64_tab.c
	rm ./data/*.exe

#

win16:
	gcc aasm.c about.c -IX:/inc/c/ -IX:/demo/template/ -o c:/bin/AASM.COM -DSIZE_MAX=0xFFFF

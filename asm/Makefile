
#rem X:/ is the unisym/
#set PATH=C:\DJGPP\BIN;C:\bin;%PATH%
#set DJGPP=C:\DJGPP\DJGPP.ENV

build:
	perl asm/todata/make-ins.pl
	perl make-macros.pl ../data/standard.mac ../data/version.mac ../data/altreg.mac  ../data/smartalign.mac ../data/outfmt/outaout.mac ../data/outfmt/outas86.mac ../data/outfmt/outbin.mac ../data/outfmt/outcoff.mac ../data/outfmt/outelf.mac ../data/outfmt/outmacho.mac ../data/outfmt/outobj.mac ../data/outfmt/outrdf.mac ../data/outfmt/outrdf2.mac
	cd todata && perl pptok.pl        h  pptok.dat ../data/pptok.h
	cd todata && perl pptok.pl        c  pptok.dat ../data/pptok.c
	cd todata && perl pptok.pl        ph pptok.dat ./perllib/pptok.ph
	cd todata && perl make-tokhash.pl c  make-ins.dat make-reg.dat tokens.dat > ../data/tokhash.c
	cd todata && perl make-tokhash.pl h  make-ins.dat make-reg.dat tokens.dat > ../data/tokens.h
	cd todata && perl make-reg.pl     h  make-reg.dat> ../data/regs.h
	cd todata && perl make-reg.pl     c  make-reg.dat> ../data/regs.c
	cd todata && perl make-reg.pl     fc make-reg.dat> ../data/regflags.c
	cd todata && perl make-reg.pl     vc make-reg.dat> ../data/regvals.c
	cd todata && perl make-reg.pl     dc make-reg.dat> ../data/regdis.c
	cd todata && perl make-reg.pl     dh make-reg.dat> ../data/regdis.h


win16:
	gcc aasm.c about.c -IX:/inc/c/ -IX:/demo/template/ -o c:/bin/AASM.COM -DSIZE_MAX=0xFFFF

c:/obj/_crc64_tab.o: ./data/_crc64_tab.c
	gcc -c -o c:/obj/_crc64_tab.o ./data/_crc64_tab.c

./data/_crc64_tab.c: ./todata/_crc64_tab.to.c
	gcc -o ./data/a.exe $<
	cd ./data/ && ./a.exe
	rm ./data/a.exe

clean:
	rm ./data/_crc64_tab.c
	rm ./data/*.exe

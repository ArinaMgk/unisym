
#rem X:/ is the unisym/
#set PATH=C:\DJGPP\BIN;C:\bin;%PATH%
#set DJGPP=C:\DJGPP\DJGPP.ENV

x86path = $(ulibpath)/cpp/Lango/Archend/x86
x86data = $(x86path)/data
x86tod = $(x86path)/todata
x86inc = $(uincpath)/cpp/Lango/Archend/x86
x86insstamp = $(x86data)/instructions.stamp
riscvpath = $(ulibpath)/cpp/Lango/Archend/riscv
riscvdata = $(riscvpath)/data
riscvtod = $(riscvpath)/todata
riscvinc = $(uincpath)/cpp/Lango/Archend/riscv
riscvinsstamp = $(riscvdata)/instructions.stamp

build: data # for linux64 [no use O3]
	cc asm/data/*.c $(x86data)/*.c asm/about.c asm/aasm*.cpp $(ulibpath)/cpp/Format/Code/*.cpp $(ulibpath)/cpp/Lango/Archend/*.cpp $(x86path)/*.cpp $(riscvpath)/*.cpp $(riscvdata)/*.cpp -I$(uincpath) -L$(ubinpath) -ll64d -lstdc++ -D_DEBUG -D_Linux64 -o $(ubinpath)/AMD64/Lin64/aasm --static -w
	cc asm/data/_*.c $(x86data)/_asm_inst.c $(x86data)/insnsb.c $(x86data)/insnsd.c $(x86data)/regs.c $(x86data)/regdis.c asm/about.c asm/dasm*.cpp asm/aasm-lib.cpp -I$(uincpath) -L$(ubinpath) -ll64d -lstdc++ -D_DEBUG -D_Linux64 -DNO_STDSCAN -o $(ubinpath)/AMD64/Lin64/dasm --static -w

#{TEMP}
build-win64: data
	cc asm/data/*.c $(x86data)/*.c asm/about.c asm/aasm*.cpp $(ulibpath)/cpp/Format/Code/*.cpp $(ulibpath)/cpp/Lango/Archend/*.cpp $(x86path)/*.cpp $(riscvpath)/*.cpp $(riscvdata)/*.cpp -I$(uincpath) -L$(ubinpath) -lw64d -lstdc++ -D_DEBUG -D_Win64 -o $(ubinpath)/AMD64/Win64/aasm --static -w
	cc asm/data/_*.c $(x86data)/_asm_inst.c $(x86data)/insnsb.c $(x86data)/insnsd.c $(x86data)/regs.c $(x86data)/regdis.c asm/about.c asm/dasm*.cpp asm/aasm-lib.cpp -I$(uincpath) -L$(ubinpath) -lw64d -lstdc++ -D_DEBUG -D_Win64 -DNO_STDSCAN -o $(ubinpath)/AMD64/Win64/dasm --static -w

data: asm/data/_crc64_tab.c $(x86insstamp) $(riscvinsstamp) asm/data/pptok.h asm/data/pptok.c asm/todata/perllib/pptok.ph asm/data/macros.c $(x86data)/tokhash.c $(x86inc)/data/tokens.h $(x86inc)/data/regs.h $(x86data)/regs.c $(x86data)/regflags.c $(x86data)/regvals.c $(x86data)/regdis.c $(x86inc)/data/regdis.h

$(x86data) $(x86inc)/data $(riscvdata) $(riscvinc)/data:
	mkdir -p $@

$(x86insstamp): $(x86tod)/make-ins.pl $(x86tod)/make-ins.dat | $(x86data) $(x86inc)/data
	perl $(x86tod)/make-ins.pl
	printf 'generated\n' > $@

$(riscvinsstamp): $(riscvtod)/make-instructions.pl $(riscvtod)/instructions.dat | $(riscvdata) $(riscvinc)/data
	perl $(riscvtod)/make-instructions.pl
	printf 'generated\n' > $@

asm/data/pptok.h:
	cd asm/todata && perl pptok.pl        h  pptok.dat ../data/pptok.h
asm/data/pptok.c:
	cd asm/todata && perl pptok.pl        c  pptok.dat ../data/pptok.c
asm/todata/perllib/pptok.ph:
	cd asm/todata && perl pptok.pl        ph pptok.dat ./perllib/pptok.ph
asm/data/macros.c:
	cd asm/todata && perl make-macros.pl ../data/standard.mac ../data/version.mac ../data/altreg.mac  ../data/smartalign.mac ../data/outfmt/outaout.mac ../data/outfmt/outas86.mac ../data/outfmt/outbin.mac ../data/outfmt/outcoff.mac ../data/outfmt/outelf.mac ../data/outfmt/outmacho.mac ../data/outfmt/outobj.mac ../data/outfmt/outrdf.mac ../data/outfmt/outrdf2.mac
$(x86data)/tokhash.c: $(x86tod)/make-tokhash.pl $(x86tod)/make-ins.dat $(x86tod)/make-reg.dat $(x86tod)/tokens.dat | $(x86data)
	perl $(x86tod)/make-tokhash.pl c $(x86tod)/make-ins.dat $(x86tod)/make-reg.dat $(x86tod)/tokens.dat > $@
$(x86inc)/data/tokens.h: $(x86tod)/make-tokhash.pl $(x86tod)/make-ins.dat $(x86tod)/make-reg.dat $(x86tod)/tokens.dat | $(x86inc)/data
	perl $(x86tod)/make-tokhash.pl h $(x86tod)/make-ins.dat $(x86tod)/make-reg.dat $(x86tod)/tokens.dat > $@
$(x86inc)/data/regs.h: $(x86tod)/make-reg.pl $(x86tod)/make-reg.dat | $(x86inc)/data
	perl $(x86tod)/make-reg.pl h $(x86tod)/make-reg.dat > $@
$(x86data)/regs.c: $(x86tod)/make-reg.pl $(x86tod)/make-reg.dat | $(x86data)
	perl $(x86tod)/make-reg.pl c $(x86tod)/make-reg.dat > $@
$(x86data)/regflags.c: $(x86tod)/make-reg.pl $(x86tod)/make-reg.dat | $(x86data)
	perl $(x86tod)/make-reg.pl fc $(x86tod)/make-reg.dat > $@
$(x86data)/regvals.c: $(x86tod)/make-reg.pl $(x86tod)/make-reg.dat | $(x86data)
	perl $(x86tod)/make-reg.pl vc $(x86tod)/make-reg.dat > $@
$(x86data)/regdis.c: $(x86tod)/make-reg.pl $(x86tod)/make-reg.dat | $(x86data)
	perl $(x86tod)/make-reg.pl dc $(x86tod)/make-reg.dat > $@
$(x86inc)/data/regdis.h: $(x86tod)/make-reg.pl $(x86tod)/make-reg.dat | $(x86inc)/data
	perl $(x86tod)/make-reg.pl dh $(x86tod)/make-reg.dat > $@


asm/data/_crc64_tab.c: asm/todata/_crc64_tab.to.c
	gcc -o asm/data/a.exe $<
	cd asm/data/ && ./a.exe
	rm asm/data/a.exe

clean:
	rm ./data/_crc64_tab.c
	rm ./data/*.exe
	rm -f $(x86data)/*.c $(x86data)/*.stamp $(x86inc)/data/*.h
	rm -f $(riscvdata)/*.cpp $(riscvdata)/*.stamp $(riscvinc)/data/*.hpp

#

win16:
	gcc aasm.c about.c -IX:/inc/c/ -IX:/demo/template/ -o c:/bin/AASM.COM -DSIZE_MAX=0xFFFF

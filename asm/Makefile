
#rem X:/ is the unisym/
#set PATH=C:\DJGPP\BIN;C:\bin;%PATH%
#set DJGPP=C:\DJGPP\DJGPP.ENV

makefilepath = $(abspath $(lastword $(MAKEFILE_LIST)))
asmpath = $(patsubst %/,%,$(dir $(makefilepath)))
projectpath = $(abspath $(asmpath)/..)

ulibpath ?= $(projectpath)/lib
uincpath ?= $(projectpath)/inc
ubinpath ?= $(abspath $(projectpath)/../_bin)

asmdata = $(asmpath)/data
asmtodata = $(asmpath)/todata
lin64out = $(ubinpath)/AMD64/Lin64
win64out = $(ubinpath)/AMD64/Win64

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

build: data | $(lin64out) # for linux64 [no use O3]
	cc $(asmdata)/*.c $(x86data)/*.c $(asmpath)/about.c $(asmpath)/aasm*.cpp $(ulibpath)/cpp/Datype/pnode.cpp $(ulibpath)/cpp/Format/Code/*.cpp $(ulibpath)/cpp/Format/Code/ELF/*.cpp $(ulibpath)/cpp/Lango/Archend/*.cpp $(x86path)/*.cpp $(riscvpath)/*.cpp $(riscvdata)/*.cpp -I$(uincpath) -L$(ubinpath) -ll64d -lstdc++ -D_DEBUG -D_Linux64 -o $(ubinpath)/AMD64/Lin64/aasm --static -w
	cc $(asmdata)/_*.c $(x86data)/_asm_inst.c $(x86data)/insnsb.c $(x86data)/insnsd.c $(x86data)/regs.c $(x86data)/regdis.c $(asmpath)/about.c $(asmpath)/dasm*.cpp $(asmpath)/aasm-lib.cpp -I$(uincpath) -L$(ubinpath) -ll64d -lstdc++ -D_DEBUG -D_Linux64 -DNO_STDSCAN -o $(ubinpath)/AMD64/Lin64/dasm --static -w

#{TEMP}
build-win64: data | $(win64out)
	cc $(asmdata)/*.c $(x86data)/*.c $(asmpath)/about.c $(asmpath)/aasm*.cpp $(ulibpath)/cpp/Datype/pnode.cpp $(ulibpath)/cpp/Format/Code/*.cpp $(ulibpath)/cpp/Format/Code/ELF/*.cpp $(ulibpath)/cpp/Lango/Archend/*.cpp $(x86path)/*.cpp $(riscvpath)/*.cpp $(riscvdata)/*.cpp -I$(uincpath) -L$(ubinpath) -lw64d -lstdc++ -D_DEBUG -D_Win64 -o $(ubinpath)/AMD64/Win64/aasm --static -w
	cc $(asmdata)/_*.c $(x86data)/_asm_inst.c $(x86data)/insnsb.c $(x86data)/insnsd.c $(x86data)/regs.c $(x86data)/regdis.c $(asmpath)/about.c $(asmpath)/dasm*.cpp $(asmpath)/aasm-lib.cpp -I$(uincpath) -L$(ubinpath) -lw64d -lstdc++ -D_DEBUG -D_Win64 -DNO_STDSCAN -o $(ubinpath)/AMD64/Win64/dasm --static -w

data: $(asmdata)/_crc64_tab.c $(x86insstamp) $(riscvinsstamp) $(asmdata)/pptok.h $(asmdata)/pptok.c $(asmtodata)/perllib/pptok.ph $(asmdata)/macros.c $(x86data)/tokhash.c $(x86inc)/data/tokens.h $(x86inc)/data/regs.h $(x86data)/regs.c $(x86data)/regflags.c $(x86data)/regvals.c $(x86data)/regdis.c $(x86inc)/data/regdis.h

$(x86data) $(x86inc)/data $(riscvdata) $(riscvinc)/data $(lin64out) $(win64out):
	mkdir -p $@

$(x86insstamp): $(x86tod)/make-ins.pl $(x86tod)/make-ins.dat | $(x86data) $(x86inc)/data
	perl $(x86tod)/make-ins.pl
	printf 'generated\n' > $@

$(riscvinsstamp): $(riscvtod)/make-instructions.pl $(riscvtod)/instructions.dat $(riscvtod)/instructions-vector.dat | $(riscvdata) $(riscvinc)/data
	perl $(riscvtod)/make-instructions.pl
	printf 'generated\n' > $@

$(asmdata)/pptok.h:
	cd $(asmtodata) && perl pptok.pl        h  pptok.dat ../data/pptok.h
$(asmdata)/pptok.c:
	cd $(asmtodata) && perl pptok.pl        c  pptok.dat ../data/pptok.c
$(asmtodata)/perllib/pptok.ph:
	cd $(asmtodata) && perl pptok.pl        ph pptok.dat ./perllib/pptok.ph
$(asmdata)/macros.c:
	cd $(asmtodata) && perl make-macros.pl ../data/standard.mac ../data/version.mac ../data/altreg.mac  ../data/smartalign.mac ../data/outfmt/outaout.mac ../data/outfmt/outas86.mac ../data/outfmt/outbin.mac ../data/outfmt/outcoff.mac ../data/outfmt/outelf.mac ../data/outfmt/outmacho.mac ../data/outfmt/outobj.mac ../data/outfmt/outrdf.mac ../data/outfmt/outrdf2.mac
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


$(asmdata)/_crc64_tab.c: $(asmtodata)/_crc64_tab.to.c
	gcc -o $(asmdata)/a.exe $<
	cd $(asmdata) && ./a.exe
	rm $(asmdata)/a.exe

clean:
	rm $(asmdata)/_crc64_tab.c
	rm $(asmdata)/*.exe
	rm -f $(x86data)/*.c $(x86data)/*.stamp $(x86inc)/data/*.h
	rm -f $(riscvdata)/*.cpp $(riscvdata)/*.stamp $(riscvinc)/data/*.hpp

#

win16:
	gcc $(asmpath)/aasm.c $(asmpath)/about.c -IX:/inc/c/ -IX:/demo/template/ -o c:/bin/AASM.COM -DSIZE_MAX=0xFFFF

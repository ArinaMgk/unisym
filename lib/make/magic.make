
OS?=l64d


ifeq ($(OS),l64d)
MAC=_Linux
else
MAC=_Win64
endif

DEFS=-D$(MAC) -D__BITS__=64 -D_DEBUG -l$(OS)

ifeq ($(OS),l64d)
DSTPATH=$(ubinpath)/AMD64/Lin64
else
DSTPATH=$(ubinpath)
endif

CXXFLAGS=$(DEFS)

MGC_CFLG=-L$(ubinpath) -std=c++17  -fno-common -static

MGC_SRCS=$(wildcard magic/*.cpp) $(wildcard magic/m_*/*.cpp) \
	lib/cpp/Lango/CloverIR.cpp

CLOVER_RISCV_SRCS=magic/backend/clover-riscv.cpp \
	lib/cpp/Lango/CloverIR.cpp \
	lib/cpp/Lango/Backend/Backend-riscv.cpp

.PHONY: all

all: $(DSTPATH)/mgc $(DSTPATH)/clover-riscv

$(DSTPATH)/mgc: $(MGC_SRCS)
	g++ $(MGC_CFLG) -o $@ $(MGC_SRCS) $(CXXFLAGS)

$(DSTPATH)/clover-riscv: $(CLOVER_RISCV_SRCS)
	g++ $(MGC_CFLG) -o $@ $(CLOVER_RISCV_SRCS) $(CXXFLAGS)

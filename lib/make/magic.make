
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
UNISYM_LIB=$(ubinpath)/lib$(OS).a

MGC_SRCS=$(wildcard magic/*.cpp) $(wildcard magic/m_*/*.cpp)

CLOVER_RISCV_SRCS=magic/backend/clover-riscv.cpp
CLOVER_OPT_SRCS=magic/backend/clover-opt.cpp

.PHONY: all

all: $(DSTPATH)/mgc $(DSTPATH)/clover-riscv $(DSTPATH)/clover-opt

$(DSTPATH)/mgc: $(MGC_SRCS) $(UNISYM_LIB)
	g++ $(MGC_CFLG) -o $@ $(MGC_SRCS) $(CXXFLAGS)

$(DSTPATH)/clover-riscv: $(CLOVER_RISCV_SRCS) $(UNISYM_LIB)
	g++ $(MGC_CFLG) -o $@ $(CLOVER_RISCV_SRCS) $(CXXFLAGS)

$(DSTPATH)/clover-opt: $(CLOVER_OPT_SRCS) $(UNISYM_LIB)
	g++ $(MGC_CFLG) -o $@ $(CLOVER_OPT_SRCS) $(CXXFLAGS)

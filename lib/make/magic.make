
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

SRCS=$(wildcard magic/*.cpp)

all:
	g++ $(MGC_CFLG) -o $(DSTPATH)/mgc $(SRCS) $(CXXFLAGS)

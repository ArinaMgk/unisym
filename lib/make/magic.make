

DEFS=-D_Linux -D__BITS__=64 -D_DEBUG -ll64d 

DSTPATH=$(ubinpath)/ELF64

CXXFLAGS=$(DEFS)

MGC_CFLG=-L$(ubinpath) -std=c++17  -fno-common -static

SRCS=$(wildcard magic/*.cpp)

all:
	g++ $(MGC_CFLG) -o $(DSTPATH)/mgc $(SRCS) $(CXXFLAGS)

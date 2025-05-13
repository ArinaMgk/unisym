

$(ubinpath)/ELF64/brancher: brancher.cpp brancher-nm.cpp
	g++ -I${uincpath} -D_DEBUG -D_Linux64 $^ -L${ubinpath} -ll64d -o $@ -static

.PHONY: clean
clean:
	rm -f $(ubinpath)/ELF64/brancher



#rem X:/ is the unisym/
#set PATH=C:\DJGPP\BIN;C:\bin;%PATH%
#set DJGPP=C:\DJGPP\DJGPP.ENV

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

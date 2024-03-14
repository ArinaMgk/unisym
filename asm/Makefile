
ain: ./data/_crc64_tab.c
	gcc -c -o c:/obj/_crc64_tab.o ./data/_crc64_tab.c

./data/_crc64_tab.c: ./data/_crc64_tab.to.c
	gcc -o ./data/a.exe ./data/_crc64_tab.to.c
	cd ./data/ && ./a.exe
	rm ./data/a.exe

clean:
	rm ./data/_crc64_tab.c
	rm ./data/*.exe

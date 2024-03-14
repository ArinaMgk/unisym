g++ -c chk_nodes.cpp -o ../../../../_obj/a.o
gcc -c ../../../lib/c/ustring/astring/StrHeapN.c -o ../../../../_obj/b.o
g++ ../../../../_obj/a.o ../../../../_obj/b.o ../../../lib/cpp/nodes/*.cpp -o ../../../../_tmp/a.exe
cd ../../../../_tmp/
a.exe
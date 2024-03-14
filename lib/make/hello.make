hello_dir=../../demo/hello/
dest_dir=../../../_tmp/
VL64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/link.exe
CSC4=C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe
nWin64:
	aasm ${hello_dir}n_Win64.asm -fwin64 -o${dest_dir}hello.win64.o
	${VL64} /subsystem:console /nodefaultlib /entry:_ento /machine:X64 "${dest_dir}hello.win64.o" kernel32.lib msvcrt.lib /out:"${dest_dir}hello.win64.exe" /LARGEADDRESSAWARE:NO
	rm ${dest_dir}hello.win64.o
StdC:
	gcc ${hello_dir}StdC.c -o ${dest_dir}hello.StdC.exe
CS:
	#INVALID ${CSC4} /nologo /out:${dest_dir}hello.DotNetCS4.exe ${hello_dir}CSharp.cs
	cd ${hello_dir} && ${CSC4} /nologo /out:${dest_dir}hello.DotNetCS4.exe CSharp.cs
Java:
	javac ${hello_dir}Java.java -d ${dest_dir}
	cd ${dest_dir} && java Java

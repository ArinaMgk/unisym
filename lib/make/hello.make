hello_dir=../../demo/hello/
dest_dir=../../../_tmp/
VL64=E:/software/VS22/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/link.exe
CSC4=C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe
Latex:
	#
Python:
	python ${hello_dir}Python/HelloPy3.py
vb:
	cmd /c VB6 /make ${hello_dir}VisualBasic6/vb6proj.vbp # /make can be omitted for /m
	mv ${hello_dir}VisualBasic6/vb6proj.exe ${dest_dir}VB6.exe # change the output name : add `ExeName32="myname.exe"` of .vbp before `[MS Transaction Server]`
Bash:
	bash ${hello_dir}Bash.sh && exit
Batch:
	cmd /C call ${hello_dir}Batch.bat
CS:
	#INVALID ${CSC4} /nologo /out:${dest_dir}hello.DotNetCS4.exe ${hello_dir}CSharp.cs
	cd ${hello_dir} && ${CSC4} /nologo /out:${dest_dir}hello.DotNetCS4.exe CSharp.cs
gas_lin:
	#
#HTML5:
Java:
	javac ${hello_dir}Java.java -d ${dest_dir}
	cd ${dest_dir} && java Java
JS:
	node ${hello_dir}JavaScript.js
Kotlin:
	kotlinc ${hello_dir}Kotlin.kt -include-runtime -d ${dest_dir}hello.Kotlin.jar
	java -jar ${dest_dir}hello.Kotlin.jar
Kot-native:
	kotlinc-native ${hello_dir}Kotlin.kt -o ${dest_dir}hello.Kotlin.exe
macroasm32:
	#
nDOS:
	#
nWin64:
	aasm ${hello_dir}n_Win64.asm -fwin64 -o${dest_dir}hello.win64.o
	${VL64} /subsystem:console /nodefaultlib /entry:_ento /machine:X64 "${dest_dir}hello.win64.o" kernel32.lib msvcrt.lib /out:"${dest_dir}hello.win64.exe" /LARGEADDRESSAWARE:NO
	rm ${dest_dir}hello.win64.o
nvcc:
	#
Perl:
	#
Rust:
	rustc ${hello_dir}Rust.rs -o ${dest_dir}hello.Rust.exe
StdC:
	gcc ${hello_dir}StdC.c -o ${dest_dir}hello.StdC.exe
C++:
	g++ ${hello_dir}StdCpp.cpp -o ${dest_dir}hello.CPP.exe

###

Go:
	# go build ${hello_dir}Go.go



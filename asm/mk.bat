REM X:/ is the unisym/
set PATH=C:\DJGPP\BIN;C:\bin;%PATH%
set DJGPP=C:\DJGPP\DJGPP.ENV
gcc aasm.c about.c -IX:/inc/c/ -IX:/demo/template/ -o c:/bin/AASM.COM -DSIZE_MAX=0xFFFF
REM X:/ is the unisym/
set PATH=C:\DJGPP\BIN;%PATH%
set DJGPP=C:\DJGPP\DJGPP.ENV
gcc aasm.c about.c   -IX:/inc/c/ -IX:/demo/template/ -o c:/bin/aasm.com -DSIZE_MAX=0xFFFF
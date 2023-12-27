title Haloword ret21
include Irvine32.inc
.data
 string byte 'Hello,World!',0
.code
mainx PROC
 mov edx, offset string
 call WriteString
 exit
mainx ENDP
end mainx

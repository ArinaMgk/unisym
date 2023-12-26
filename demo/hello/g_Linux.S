# Arina RFA030934 GNU-AS (HelloLinuxGAs.S)
.data # section declaration
msg:
 .ascii "Hello, world!\n" 
 len = . - msg
.text
 .global _start
_start:
 movl $len,%edx # third argument: message length
 movl $msg,%ecx # second argument: pointer to message
 movl $1,%ebx # first argument: file handle (stdout)
 movl $4,%eax # system call number (sys_write)
 int $0x80
#exit
 movl $0,%ebx # exit code
 movl $1,%eax # (sys_exit)
 int $0x80 # call kernel
#use $as and $ls to make object file.
; ASCII ANY TAB4 CRLF
;

[BITS 32]

; _unlock: mov [lock_var], 0

; _lock:
GLOBAL LockSwap8; byte LockSwap8(byte* b, byte val);
LockSwap8:; STACK-> ADDR b val
	MOV EAX, [EBP+4*2]
	LOCK XCHG [EBP+4*1], AL; or use test&set
RET

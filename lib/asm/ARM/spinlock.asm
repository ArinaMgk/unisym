; _lock:
	loop:
	LDREX R1, [lock_var]
	CMP R1, #0
	BEQ acquire
	B loop
	acquire:
	MOV R2, #1
	STREX R3, R2, [lock_var]
	CMP R3, #0
	BNE loop
; _unlock:
	MOV R0, #0
	STR R0, [lock_var]

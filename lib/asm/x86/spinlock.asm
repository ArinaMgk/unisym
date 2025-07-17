; _lock: lock xchg [lock_var], eax (or use test&set)

; _unlock: mov [lock_var], 0

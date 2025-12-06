; _lock: lock xchg [lock_var], eax (or use test&set)

; _unlock: mov [lock_var], 0

; mutex: os-layer, sleep if the lock is locked by another thread
; spinlock: usr-layer, busy-wait or give-up if the lock is locked by another thread

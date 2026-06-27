	arrayA byte 10h, 20h;
	pByte typedef ptr byte;
	ptr1 pByte arrayA;
	mov esi,ptr1
	mov al,[esi]

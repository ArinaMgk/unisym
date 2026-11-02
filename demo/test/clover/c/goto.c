// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Goto Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

int main() {
	int value = 0;

	goto initialize;
dead:
	value = 100;

initialize: {
	value = 4;
}

repeat:
	value += 2;
	if (value < 10) goto repeat;
	goto finish;

unused: ;
	value = 99;

finish:
	return value;
}

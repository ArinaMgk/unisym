// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Declaration Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

typedef unsigned long Size;
typedef int Row[3];
typedef int (*RowPointer)[3];
typedef int (*Unary)(int);

int add(int, int);
int* identity(int*);
int row_sum(int (*row)[3]);
int increment(int);
int apply(int function(int), int);
int (*select_unary(void))(int);
int (*select_row(void))[3];

Unary default_unary = increment;

int add(int left, int right) {
	return left + right;
}

int* identity(int* value) {
	return value;
}

int row_sum(int (*row)[3]) {
	return row[0][0] + row[0][1] + row[0][2];
}

int increment(int value) {
	return value + 1;
}

int apply(int function(int), int value) {
	return (*function)(value);
}

int (*select_unary(void))(int) {
	return increment;
}

int main(void) {
	Row row = { 1, 2, 3 };
	RowPointer pointer = &row;
	int (*callbacks[2])(int), (*fallback)(int) = default_unary;
	callbacks[0] = fallback;
	Size layout_size = sizeof(Row) + sizeof(RowPointer);
	typedef int Scalar;
	Scalar value = 4;
	{
		typedef char Scalar;
		Scalar byte_value = 5;
		value += sizeof(byte_value);
	}
	return layout_size + row_sum(pointer) + *identity(&value) + (*pointer)[1] +
		apply(callbacks[0], 7) + select_unary()(8);
}

// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Static Relocation Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

struct Item {
	char tag;
	int value;
	int values[2];
};

struct Holder {
	int* first;
	int* third;
	int* member;
	int* nested;
};

struct Pointer {
	int* value;
};

int values[4] = {1, 2, 3, 4};
struct Item item = {1, 5, {7, 9}};

int add1(int value) {
	return value + 1;
}

int* root = values + 2;
struct Holder holder = {
	.first = values,
	.third = values + 2,
	.member = &item.value,
	.nested = &item.values[1]
};
int* table[] = {values + 1, &item.value};
int (*function_table[1])(int) = {add1};
struct Pointer anonymous = {
	.value = (int[]){7, 9, 11} + 1
};

int static_value() {
	static struct Holder local = {
		.first = &values[1],
		.third = values + 2,
		.member = &item.value,
		.nested = &item.values[1]
	};
	return *local.first + *local.third + *local.member + *local.nested;
}

int main() {
	return *root + *holder.first + *holder.third + *holder.member +
		*holder.nested + *table[0] + *table[1] + *anonymous.value +
		function_table[0](4) + static_value();
}

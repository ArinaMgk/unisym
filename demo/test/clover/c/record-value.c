// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Record Value Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

struct Pair {
	int left;
	int right;
};

struct Nested {
	struct Pair pair;
	int tail;
};

union Number {
	int word;
	char bytes[4];
};

packed struct PackedPair {
	char tag;
	int value;
};

int main() {
	struct Pair first = {1, 2};
	struct Pair second = {3, 4};
	struct Pair third = {5, 6};
	struct Pair copied = {0, 0};
	struct Nested nested = {7, 8, 9};
	struct Nested nested_copy = {{0, 0}, 0};
	union Number number = {10};
	union Number number_copy = {0};
	struct PackedPair packed = {11, 12};
	struct PackedPair packed_copy = {0, 0};

	first = second;
	first = second = third;
	copied = (first = third);
	nested_copy = nested;
	number_copy = number;
	packed_copy = packed;

	return
		(first.left == 5) + (first.right == 6) +
		(second.left == 5) + (second.right == 6) +
		(copied.left == 5) + (copied.right == 6) +
		((first = third).left == 5) +
		(nested_copy.pair.left == 7) +
		(nested_copy.pair.right == 8) + (nested_copy.tail == 9) +
		(number_copy.word == 10) +
		(packed_copy.tag == 11) + (packed_copy.value == 12);
}

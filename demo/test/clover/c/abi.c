// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend ABI Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

int sum10(int a, int b, int c, int d, int e,
	int f, int g, int h, int i, int j) {
	return a + b + c + d + e + f + g + h + i + j;
}

struct Bytes {
	char first;
	char second;
	char third;
};

struct Pair64 {
	long first;
	long second;
};

struct Triple64 {
	long first;
	long second;
	long third;
};

struct FloatPair {
	double first;
	double second;
};

union Word {
	long integer;
	double floating;
};

struct MixedValue {
	long integer;
	double real;
};

struct Pair64 make_pair(long first, long second);

struct Pair64 make_pair(long first, long second) {
	struct Pair64 value = {first, second};
	return value;
}

struct Bytes make_bytes() {
	struct Bytes value = {1, 2, 3};
	return value;
}

struct Triple64 make_triple(long first, long second, long third) {
	struct Triple64 value = {first, second, third};
	return value;
}

struct FloatPair make_float_pair(double first, double second) {
	struct FloatPair value = {first, second};
	return value;
}

union Word make_word(long value) {
	union Word result;
	result.integer = value;
	return result;
}

struct MixedValue make_mixed(long integer, double real) {
	struct MixedValue result = {integer, real};
	return result;
}

int sum_bytes(struct Bytes value) {
	return value.first + value.second + value.third;
}

long sum_pair(struct Pair64 value) {
	return value.first + value.second;
}

long sum_triple(struct Triple64 value) {
	return value.first + value.second + value.third;
}

int pair_after_seven(int a, int b, int c, int d, int e, int f, int g,
	struct Pair64 value, int tail) {
	return (int)(value.first + value.second) + a + b + c + d + e + f + g + tail;
}

int pair_after_eight(int a, int b, int c, int d, int e, int f, int g, int h,
	struct Pair64 value, int tail) {
	return (int)(value.first + value.second) + a + b + c + d + e + f + g + h + tail;
}

int main() {
	struct Bytes bytes = {1, 2, 3};
	struct Pair64 pair = {4, 5};
	struct Triple64 triple = {6, 7, 8};
	struct Pair64 returned_pair = make_pair(11, 12);
	struct Triple64 returned_triple = make_triple(13, 14, 15);
	struct FloatPair returned_floats = make_float_pair(5.0, 6.0);
	union Word returned_union = make_word(12);
	struct MixedValue returned_mixed = make_mixed(7, 8.0);
	return sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) +
		sum_bytes(bytes) + (int)sum_pair(pair) + (int)sum_triple(triple) +
		pair_after_seven(1, 1, 1, 1, 1, 1, 1, pair, 1) +
		pair_after_eight(1, 1, 1, 1, 1, 1, 1, 1, pair, 1) +
		(int)(returned_pair.first + returned_pair.second) +
		(int)(returned_triple.first + returned_triple.second + returned_triple.third) +
		sum_bytes(make_bytes()) + (int)sum_pair(make_pair(1, 2)) +
		(int)returned_floats.first + (int)returned_floats.second +
		(int)returned_union.integer + (int)returned_mixed.integer +
		(int)returned_mixed.real;
}

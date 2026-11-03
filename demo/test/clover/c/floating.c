// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Floating ABI Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

struct MixedValue {
	float floating;
	int integer;
};

struct FloatPair {
	double wide;
	float narrow;
};

long double global_long_double = 1.5L;

float add_single(float left, float right) {
	return left + right;
}

double mix_precision(float narrow, double wide, int integer) {
	return add_single(narrow, 1.5f) + wide + integer;
}

int sum_mixed(struct MixedValue value) {
	return (int)value.floating + value.integer;
}

int sum_pair(struct FloatPair value) {
	return (int)(value.wide + value.narrow);
}

long double scale_long_double(long double value) {
	return value * 2.0L + 1;
}

int variadic_head(int value, ...) {
	return value;
}

int main() {
	struct MixedValue mixed;
	struct FloatPair pair;
	float values[2] = {0x1.4p0f, 2.75f};
	float* value_pointer = values;
	unsigned char byte = 255;
	short negative_one = -1;
	unsigned int one = 1;
	long negative_two = -2;
	float single = 1.5f;
	double wide = 2.5;
	unsigned long shift = 3;
	mixed.floating = 2.5f;
	mixed.integer = 3;
	pair.wide = 2.25;
	pair.narrow = 1.75f;
	return sum_mixed(mixed) + sum_pair(pair) +
		(int)scale_long_double(global_long_double) + sizeof(long double) +
		variadic_head(7, (char)2, 3.0f) +
		(int)(1.5f + 2.25) +
		(int)(value_pointer[0] + value_pointer[1] +
			sizeof(float) + sizeof(double)) +
		(int)mix_precision(1.25f, 2.5, 3) +
		(byte + negative_one == 254) + (negative_two + one == -1) +
		(int)(single + wide) + (1 << shift);
}

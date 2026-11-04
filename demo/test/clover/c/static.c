// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Static Storage and Linkage Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

static int file_value = 2;
static double file_scale = 1.5;
static int file_slots[2];

static int add_file_value(int value) {
	return value + file_value;
}

static int factorial(int value) {
	if (value <= 1) return 1;
	return value * factorial(value - 1);
}

int next_value() {
	static int value = 2;
	value++;
	return value;
}

int next_zero() {
	static int value;
	return ++value;
}

int next_slot() {
	file_slots[0]++;
	return file_slots[0];
}

int shadowed_static(int select_first) {
	if (select_first) {
		static int value = 10;
		return ++value;
	}
	{
		static int value = 20;
		return ++value;
	}
}

int static_array_value() {
	static int values[3] = {3, 4, 5};
	values[1]++;
	return values[0] + values[1] + values[2];
}

int main() {
	int first = next_value();
	int second = next_value();
	int zero_first = next_zero();
	int zero_second = next_zero();
	int slot_first = next_slot();
	int slot_second = next_slot();
	int shadow_first = shadowed_static(1);
	int shadow_second = shadowed_static(0);
	int shadow_first_again = shadowed_static(1);
	int shadow_second_again = shadowed_static(0);
	int array_first = static_array_value();
	int array_second = static_array_value();
	return add_file_value(3) + first * 10 + second +
		zero_first + zero_second + slot_first + slot_second +
		shadow_first + shadow_second + shadow_first_again +
		shadow_second_again + array_first + array_second +
		factorial(4) + (int)file_scale;
}

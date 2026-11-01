// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Operator Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

int main() {
	int score = 0;
	int value;
	int left;
	int right;
	unsigned int unsigned_value;
	char byte;
	int values[3] = {2, 4, 6};
	int* pointer = values;

	score = score + (20 - 13 + 12 == 19);
	score = score + (1 - 8 / (2 * 2) + 3 * 6 == 17);
	score = score + (-7 % 3 == -1);
	score = score + (- -12 * +4 == 48);
	score = score + (-5 == -2 - 3);
	score = score + (2 * 3 != 2 + 3);
	score = score + (2 < 3);
	score = score + (3 <= 3);
	score = score + (-2 > -3);
	score = score + (-3 >= -3);

	unsigned_value = 0;
	unsigned_value = unsigned_value - 1;
	score = score + (unsigned_value > 1);
	score = score + (unsigned_value / 16777216 == 255);
	score = score + ((12 & 10) == 8);
	score = score + ((12 ^ 10) == 6);
	score = score + ((12 | 10) == 14);
	score = score + ((3 << 4) == 48);
	score = score + ((-8 >> 2) == -2);
	score = score + ((unsigned_value >> 28) == 15);
	score = score + (!0 == 1);
	score = score + (!7 == 0);
	score = score + (~~5 == 5);
	score = score + (~0 == -1);

	value = 10;
	value += 5;
	score = score + (value == 15);
	value -= 3;
	score = score + (value == 12);
	value *= 4;
	score = score + (value == 48);
	value /= 6;
	score = score + (value == 8);
	value %= 3;
	score = score + (value == 2);
	value <<= 3;
	score = score + (value == 16);
	value >>= 2;
	score = score + (value == 4);
	value &= 6;
	score = score + (value == 4);
	value ^= 3;
	score = score + (value == 7);
	value |= 8;
	score = score + (value == 15);
	byte = 120;
	byte += 10;
	score = score + (byte == -126);

	value = 3;
	score = score + (value++ == 3);
	score = score + (value == 4);
	score = score + (++value == 5);
	score = score + (value-- == 5);
	score = score + (value == 4);
	score = score + (--value == 3);

	(*pointer++) += 3;
	score = score + (values[0] == 5);
	score = score + (*pointer == 4);
	pointer += 1;
	score = score + (*pointer == 6);
	pointer -= 1;
	score = score + (*pointer == 4);
	score = score + (!pointer == 0);
	score = score + ((value *= 4) == 12);
	left = 1;
	right = 3;
	left += right *= 2;
	score = score + (left == 7) + (right == 6) - 1;

	value = 0;
	score = score + ((0 && (value = 1)) == 0);
	score = score + (value == 0);
	score = score + ((1 || (value = 2)) == 1);
	score = score + (value == 0);
	score = score + ((1 && (value = 3)) == 1);
	score = score + (value == 3);
	score = score + ((0 || (value = 4)) == 1);
	score = score + (value == 4);
	score = score + ((1 && 2 && 3) == 1);
	score = score + ((0 || 0 || 5) == 1);

	score = score + ((1 ? 7 : 9) == 7);
	score = score + ((0 ? 7 : 9) == 9);
	value = 1 ? 5 : 6;
	score = score + (value == 5);
	value = 0;
	left = 1 ? (value = 7) : (value = 8);
	score = score + (left == 7);
	score = score + (value == 7);
	score = score + ((1 ? 0 ? 2 : 3 : 4) == 3);
	score = score + ((0 ? 2 : 1 ? 4 : 5) == 4);
	score = score + ((1 ? (value = 1, value + 2) : 0) == 3);
	pointer = 1 ? &values[0] : &values[1];
	score = score + (*pointer == 5);

	value = 257;
	score = score + ((char)value == 1);
	score = score + ((unsigned char)-1 == 255);
	score = score + ((short)65537 == 1);
	score = score + ((int)(char)value == 1);
	score = score + ((int)-(char)value == -1);
	(void)(value = 9);
	score = score + (value == 9);
	pointer = (int*)(char*)values;
	score = score + (*pointer == 5);
	pointer = (int*)(unsigned long long)values;
	score = score + (*pointer == 5);
	pointer = (int*)0;
	score = score + ((unsigned long long)pointer == 0);
	score = score + ((long long)(int*)-1 == -1);

	return score;
}

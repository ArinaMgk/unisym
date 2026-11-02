// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Array Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

int sum_vector(int values[], int count) {
	int index;
	int total = 0;

	for (index = 0; index < count; index++) total += values[index];
	return total;
}

int sum_row(int values[][4], int row) {
	return values[row][0] + values[row][1] +
		values[row][2] + values[row][3];
}

int last(int values[3]) {
	return values[2];
}

int main() {
	int vector[3] = {1, 2, 3};
	int matrix[2][4] = {1, 2, 3, 4, 5, 6, 7, 8};

	return sum_vector(vector, 3) + sum_row(matrix, 1) +
		matrix[0][2] + last(vector);
}

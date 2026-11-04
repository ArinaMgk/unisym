// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Array Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

int global_vector[] = {2, 3, 4};
char global_text[] = "AZ";
unsigned char global_bytes[4] = {"B"};

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
	int inferred[] = {4, 5, 6};
	int partial[][3] = {{7}, {8, 9}};
	char local_text[5] = "Hi";
	const char exact_text[2] = "OK";
	unsigned char bytes[] = "A";
	static char static_text[] = "S";

	return sum_vector(vector, 3) + sum_row(matrix, 1) +
		matrix[0][2] + last(vector) + global_vector[2] +
		(global_text[1] == 'Z') + (global_bytes[0] == 'B') +
		inferred[2] + (partial[0][1] == 0) + partial[1][1] +
		(local_text[2] == 0) + (exact_text[1] == 'K') +
		(bytes[1] == 0) + (static_text[0] == 'S') + sizeof(global_text);
}

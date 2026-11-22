// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Variable-Length Array Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

struct VlaRecord {
	int value;
};

int leave_vla_scope(int count) {
	int values[count];
	values[0] = 9;
	return values[0];
}

struct VlaRecord return_vla_record(int count) {
	struct VlaRecord values[count];
	values[0].value = 11;
	return values[0];
}

int read_matrix(int rows, int columns, int values[rows][columns]) {
	return values[rows - 1][columns - 1] + sizeof(*values);
}

int read_cube(int planes, int rows, int columns,
	int values[planes][rows][columns]) {
	return values[planes - 1][rows - 1][columns - 1] + sizeof(*values);
}

int parameter_bound_once(int count, int values[2][count++]) {
	return count + sizeof(*values);
}

int goto_leave_inner_scope(int count) {
	int result = 0;
	int outer[count];
	outer[0] = 4;
	{
		int inner[count + 1];
		inner[0] = 6;
		result = inner[0];
		goto keep_outer;
	}
keep_outer:
	return outer[0] + result;
}

int goto_leave_nested_scopes(int count) {
	int result = 0;
	{
		int outer[count];
		outer[0] = 13;
		{
			int inner[count + 1];
			inner[0] = 17;
			result = outer[0] + inner[0];
			goto finished;
		}
	}
finished:
	return result;
}

int goto_reenter_vla_scope(int count) {
	int pass = 0;
	int result = 0;
again: ;
	int values[count + pass];
	values[0] = 5 + pass;
	result += values[0];
	pass++;
	if (pass < 2) goto again;
	return result;
}

int goto_within_vla_scope(int count) {
	int values[count];
	int index = 0;
again:
	values[0] = ++index;
	if (index < 3) goto again;
	return values[0];
}

int main() {
	int score = 0;
	int count = 3;
	int values[count++];

	values[2] = 7;
	score += values[2];
	score += sizeof(values);
	score += count;
	score += sizeof(int[count++]);
	score += count;

	{
		int nested[count];
		nested[4] = 6;
		score += nested[4];
	}

	int index = 0;
	while (index < 4) {
		int iteration[index + 1];
		iteration[index] = index + 1;
		if (index == 1) {
			score += iteration[index];
			index++;
			continue;
		}
		if (index == 3) {
			score += iteration[index];
			break;
		}
		score += iteration[index];
		index++;
	}

	for (int loop_storage[count]; index < 6; index++) {
		int body_storage[index + 1];
		body_storage[index] = index;
		if (index == 4) {
			score += body_storage[index];
			continue;
		}
		score += body_storage[index];
	}

	do {
		int once[count];
		once[0] = 1;
		score += once[0];
	} while (0);

	int rows = 2;
	int columns = 3;
	int matrix[rows][columns];
	matrix[1][2] = 8;
	score += matrix[1][2];
	score += sizeof(matrix);
	score += sizeof(matrix[0]);
	score += read_matrix(rows, columns, matrix);

	int (*row)[columns] = matrix;
	row++;
	score += row[0][2];
	score += sizeof(*row);

	int planes = 2;
	int cube[planes][rows][columns];
	cube[1][1][2] = 5;
	score += cube[1][1][2];
	score += sizeof(cube[0]);
	score += sizeof(cube[0][0]);
	score += read_cube(planes, rows, columns, cube);

	int fixed[2][3];
	fixed[1][2] = 4;
	if (read_matrix(2, 3, fixed) != 16) return 1;
	if (parameter_bound_once(3, fixed) != 16) return 2;

	int bound_rows = 2;
	int bound_columns = 3;
	int evaluated[bound_rows++][bound_columns++];
	if (bound_rows != 3 || bound_columns != 4 ||
		sizeof(evaluated) != 24) return 3;
	if (goto_leave_inner_scope(2) != 10) return 4;
	if (goto_leave_nested_scopes(2) != 30) return 5;
	if (goto_reenter_vla_scope(2) != 11) return 6;
	if (goto_within_vla_scope(2) != 3) return 7;

	return score + leave_vla_scope(2) + return_vla_record(2).value;
}

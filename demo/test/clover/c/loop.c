// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Loop Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

int main() {
	int score = 0;
	int i = 0;
	int j;

	while (i < 8) {
		i++;
		if (i == 2) continue;
		if (i == 6) break;
		score += i;
	}

	for (i = 0; i < 7; i++) {
		if (i == 1) continue;
		if (i == 5) break;
		score += i;
	}

	for (i = 0; i < 3; i++) {
		j = 0;
		while (j < 5) {
			j++;
			if (j == 2) continue;
			if (j == 4) break;
			score++;
		}
	}

	while (1) break;
	for (i = 0; i < 1; i++) continue;

	i = 0;
	do {
		i++;
		if (i == 1) continue;
		score++;
	} while (i < 3);

	for (int k = 0; k < 4; k++) {
		if (k == 1) continue;
		score += k;
	}

	int k = 7;
	for (int k = 0; k < 2; k++) score += k;
	score += k;

	return score;
}

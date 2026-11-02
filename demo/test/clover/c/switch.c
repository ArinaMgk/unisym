// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Switch Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

enum Mode {
	ModeFirst = 1,
	ModeSecond = 4
};

int Classify(int value) {
	int result = 0;

	switch (value) {
	case 0:
		result = 1;
		break;
	case 1:
	case 2:
		result = 4;
		break;
	default:
		result = 8;
	}

	return result;
}

int main() {
	int score = Classify(0) + Classify(2) + Classify(9);
	int value = 0;
	int i = 0;

	// Verify ordinary fallthrough across multiple case labels.
	switch (2) {
	case 1:
		value = 10;
	case 2:
		value += 3;
	case 3:
		value += 5;
	}
	score += value;

	// Each break leaves the nearest switch, including a nested switch.
	switch (ModeFirst) {
	case ModeFirst:
		switch (ModeSecond) {
		case ModeSecond:
			score += 16;
			break;
		default:
			score = 100;
		}
		score += 2;
		break;
	default:
		score = 100;
	}

	// Continue still belongs to the enclosing loop, not to switch.
	while (i < 4) {
		i++;
		switch (i) {
		case 2:
			continue;
		case 3:
			break;
		default:
			score += 4;
		}
		score++;
	}

	// The controlling expression is evaluated exactly once.
	value = 0;
	switch (value++) {
	case 0:
		break;
	default:
		value = 100;
	}

	switch (0);
	switch (0) {}
	return score + value;
}

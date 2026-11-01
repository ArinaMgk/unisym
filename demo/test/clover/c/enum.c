// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Enumeration Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

enum Color {
	ColorRed,
	ColorGreen = 4,
	ColorBlue
};

enum State {
	StateNegative = -1,
	StateZero = !3,
	StateAllBits = ~0,
	StateLetter = 'A',
};

enum Offset {
	OffsetFirst = ColorBlue,
	OffsetSecond
};

enum Sequence {
	SequenceBase = 3,
	SequenceAlias = SequenceBase,
	SequenceNext
};

enum Forward;

enum Forward* forward_pointer;

enum Forward {
	ForwardValue = 6
};

enum Combined {
	CombinedBase = 2,
	CombinedExpression = ((CombinedBase + 3) * 4) << 1,
	CombinedValue = CombinedExpression == 40 && !0 ? 7 : 0
} combined_value = CombinedValue;

enum Wide {
	WideValue = 2147483648U
};

enum Huge {
	HugeValue = 4294967296L
};

enum Huge huge_value = HugeValue;

enum Color selected_color = ColorBlue;

enum Color PassColor(enum Color color) {
	return color;
}

enum Huge PassHuge(enum Huge value) {
	return value;
}

int BlockEnumScore() {
	enum Scope {
		ScopeValue = 3
	} outer_value = ScopeValue;
	int score = outer_value;
	{
		enum Scope {
			ScopeValue = 8,
			ScopeNext = ScopeValue + 1
		} inner_value = ScopeNext;
		score += ScopeValue + inner_value;
	}
	return score + ScopeValue;
}

int main() {
	enum Color local_color = ColorGreen;
	enum Offset offset = OffsetSecond;
	return selected_color + local_color + PassColor(ColorRed) + offset +
		sizeof(enum Color) + ((enum Color)4 == ColorGreen) +
		(StateNegative == -1) + (StateZero == 0) + (StateAllBits == -1) +
		(StateLetter == 65) + (SequenceAlias == 3) + (SequenceNext == 4) +
		ForwardValue + combined_value + (sizeof(enum Wide) == 4) +
		(sizeof(enum Huge) == 8) + (WideValue > 0) +
		(PassHuge(huge_value) == HugeValue) + BlockEnumScore();
}

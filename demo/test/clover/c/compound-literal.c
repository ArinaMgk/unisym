// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Compound Literal Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

struct Pair {
	int left;
	int right;
};

union Number {
	int word;
	char byte;
};

int* global_scalar = &(int){2};
int* global_values = (int[]){3, 5, 7};
struct Pair* global_pair = &(struct Pair){.left = 11, .right = 13};
const int* global_constant = &(const int){17};
int named_global = 19;
int* named_global_address = &named_global;

int main() {
	int* scalar = &(int){7};
	int* values = (int[]){1, 2, 3, 4};
	struct Pair* pair = &(struct Pair){.right = 11, .left = 5};
	union Number* number = &(union Number){.word = 13};
	struct Pair copy = (struct Pair){.left = 17, .right = 19};
	struct Pair nested = (struct Pair){
		.left = (int){23},
		.right = (int[]){29, 31}[1]
	};
	global_values[1]++;
	global_pair->left += 1;
	return *scalar + values[2] + pair->left + pair->right + number->word +
		copy.left + copy.right + nested.left + nested.right + *global_scalar +
		global_values[1] + global_pair->left + global_pair->right +
		*global_constant + *named_global_address;
}

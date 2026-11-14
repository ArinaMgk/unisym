// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Aggregate Initializer Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

struct Pair {
	int left;
	int right;
};

struct Packet {
	char tag;
	struct Pair pair;
	int values[3];
};

struct Holder {
	struct Pair pair;
	int tail;
};

union Number {
	int word;
	char bytes[4];
};

packed struct PackedValue {
	char tag;
	int value;
};

struct Packet global_packet = {1, {1, 1}, {1, 1}};
struct Pair global_pairs[] = {{1, 2}, {3}};
union Number global_number = {1};
struct PackedValue global_packed = {1, 1};
struct Packet designated_global = {
	.values[2] = 5,
	.pair.right = 4,
	.tag = 1,
	.pair.left = 3
};
int designated_values[] = {[3] = 7, [1] = 5, 6};
union Number designated_number = {.bytes = {8, 9}};

int static_score() {
	static struct Packet packet = {1, {1, 1}, {1, 1}};
	static union Number number = {1};
	static struct Pair designated_pair = {.right = 3, .left = 2};
	return packet.tag + packet.pair.left + packet.pair.right +
		packet.values[0] + packet.values[1] + packet.values[2] + number.word +
		designated_pair.left + designated_pair.right;
}

int main() {
	struct Pair partial = {3};
	struct Packet flat = {1, 2, 3, 4, 5};
	struct Packet nested = {2, {6}, {7}};
	struct Pair matrix[2][2] = {{{8, 9}, {10}}, {11, 12, 13, 14}};
	union Number first = {15};
	union Number numbers[2] = {{16}, {17}};
	struct PackedValue packed_value = {18, 19};
	struct Pair copied = partial;
	struct Holder holder = {partial, 2};
	union Number copied_number = first;
	struct Pair inferred[] = {{1, 2}, {3, 4}};
	struct Packet designated_local = {
		.pair.right = 4,
		.values[2] = 5,
		.tag = 1,
		.pair.left = 3
	};
	int overwritten[2] = {[1] = 2, [1] = 5};
	struct Pair continued[2] = {[0].right = 6, 7, 8};
	return partial.left + partial.right +
		flat.tag + flat.pair.left + flat.pair.right +
		flat.values[0] + flat.values[1] + flat.values[2] +
		nested.tag + nested.pair.left + nested.pair.right +
		nested.values[0] + nested.values[1] + nested.values[2] +
		matrix[0][0].left + matrix[0][0].right +
		matrix[0][1].left + matrix[0][1].right +
		matrix[1][0].left + matrix[1][0].right +
		matrix[1][1].left + matrix[1][1].right +
		first.word + numbers[0].word + numbers[1].word +
		packed_value.tag + packed_value.value +
		global_packet.tag + global_packet.pair.left + global_packet.pair.right +
		global_packet.values[0] + global_packet.values[1] +
		global_packet.values[2] + global_number.word +
		global_pairs[0].left + global_pairs[0].right +
		global_pairs[1].left + global_pairs[1].right +
		global_packed.tag + global_packed.value + static_score() +
		copied.left + copied.right + holder.pair.left + holder.pair.right +
		holder.tail + copied_number.word +
		inferred[0].left + inferred[0].right +
		inferred[1].left + inferred[1].right +
		(sizeof(global_pairs) == 16) + (sizeof(inferred) == 16) +
		designated_global.tag + designated_global.pair.left +
		designated_global.pair.right + designated_global.values[2] +
		designated_values[1] + designated_values[2] + designated_values[3] +
		sizeof(designated_values) + designated_number.bytes[0] +
		designated_number.bytes[1] + designated_local.tag +
		designated_local.pair.left + designated_local.pair.right +
		designated_local.values[2] + overwritten[1] +
		continued[0].right + continued[1].left + continued[1].right;
}

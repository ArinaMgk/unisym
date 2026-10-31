// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Record Layout Test
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

struct Header {
	char tag;
	int length;
	char flags;
};

struct WideValue {
	char prefix;
	short code;
	long payload;
};

struct ScalarOrder {
	long long wide;
	char byte;
	short half;
	int word;
};

struct Packet {
	struct Header headers[2];
	int samples[3];
};

union Number {
	int word;
	char bytes[4];
};

union Container {
	struct Header header;
	long long wide;
};

packed struct PackedValue {
	char prefix;
	short half;
	int word;
	long long wide;
};

packed union PackedUnion {
	char bytes[3];
	short half;
};

padded struct PaddedValue {
	char prefix;
	int value;
};

alignb(16) packed struct ByteAlignedPacked {
	char prefix;
	int value;
};

aligne(4) padded struct ExponentAlignedPadded {
	char prefix;
	int value;
};

struct Link;

struct Link {
	int value;
	struct Link* next;
};

int main() {
	struct Header header;
	struct WideValue wide_value;
	struct ScalarOrder scalar_order;
	struct Header* header_pointer = &header;
	struct WideValue* wide_pointer = &wide_value;
	struct Packet packets[2];
	struct Packet* packet_pointer = packets;
	struct Header matrix[2][2];
	union Number number;
	union Number numbers[2];
	union Number* number_pointer = &number;
	union Container container;
	struct PackedValue packed_values[2];
	union PackedUnion packed_union;
	struct PaddedValue padded_value;
	struct ByteAlignedPacked byte_aligned[2];
	struct ExponentAlignedPadded exponent_aligned;
	struct Link first_link;
	struct Link second_link;
	int* packed_word_pointer = &packed_values[1].word;
	header.tag = 1;
	header_pointer->length = 20;
	header.flags = 2;
	wide_pointer->payload = 8;
	scalar_order.word = 13;
	packets[1].headers[1].length = 6;
	packet_pointer->samples[2] = 4;
	matrix[1][1].tag = 5;
	number_pointer->word = 7;
	numbers[1].word = 9;
	container.header.length = 6;
	packed_values[1].half = -2;
	packed_values[1].word = 305419896;
	packed_values[1].wide = 1234567;
	packed_union.half = 257;
	padded_value.value = 13;
	byte_aligned[1].value = 17;
	exponent_aligned.value = 19;
	second_link.value = 11;
	first_link.next = &second_link;
	return header.tag + *&header.length + header.flags +
		wide_pointer->payload + scalar_order.word +
		packets[1].headers[1].length + packet_pointer->samples[2] +
		matrix[1][1].tag + sizeof(header.tag) + sizeof(header.length) +
		sizeof(struct Header[3]) + sizeof(packets) + number.word +
		numbers[1].word + container.header.length + sizeof(number) +
		sizeof(union Number[3]) + sizeof(container) +
		sizeof(packed_values[0]) + (packed_values[1].half == -2) +
		(*packed_word_pointer == 305419896) +
		(packed_values[1].wide == 1234567) + sizeof(packed_union) +
		(packed_union.half == 257) + (sizeof(padded_value) == 8) +
		(sizeof(byte_aligned[0]) == 16) + (byte_aligned[1].value == 17) +
		(sizeof(exponent_aligned) == 16) + (exponent_aligned.value == 19) +
		(first_link.next->value == 11);
}

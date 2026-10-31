// ASCII C99 TAB4 LF
// Docutitle: Clover C Frontend Structure Layout Test
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

int main() {
	struct Header header;
	struct WideValue wide_value;
	struct ScalarOrder scalar_order;
	struct Header* header_pointer = &header;
	struct WideValue* wide_pointer = &wide_value;
	header.tag = 1;
	header_pointer->length = 20;
	header.flags = 2;
	wide_pointer->payload = 8;
	scalar_order.word = 13;
	return header.tag + header_pointer->length + header.flags +
		wide_pointer->payload + scalar_order.word;
}

// ASCII C99 TAB4 LF
// Docutitle: Table of toupper and tolower
// Codifiers: @ArinaMgk
// OpLicense: http://unisym.org/license.html

// C Style Array will be treated as pointer in return expression
#include "../../../inc/cpp/ISO_IEC_STD/array"

constexpr auto make_tolower() {
	uni::Array<unsigned char, 256> t{};
	for (int i = 0; i < 256; ++i) {
		char c = static_cast<char>(i);
		t[i] = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
	}
	return t;
}

constexpr auto make_toupper() {
	uni::Array<unsigned char, 256> t{};
	for (int i = 0; i < 256; ++i) {
		char c = static_cast<char>(i);
		t[i] = (c >= 'a' && c <= 'z') ? (c - 32) : c;
	}
	return t;
}

constexpr auto make_alnum_digit() {
	uni::Array<unsigned char, 128> t{};
	for (int i = 0; i < t.size(); ++i) {
		char c = static_cast<char>(i);
		if (c >= '0' && c <= '9') t[i] = c - '0';
		else if (c >= 'A' && c <= 'Z') t[i] = c - 'A' + 10;
		else if (c >= 'a' && c <= 'z') t[i] = c - 'a' + 10;
		else t[i] = 0;
	}
	return t;
}

_ESYM_C constexpr auto _tab_tolower     = make_tolower();
_ESYM_C constexpr auto _tab_toupper     = make_toupper();
_ESYM_C constexpr auto _tab_alnum_digit = make_alnum_digit();

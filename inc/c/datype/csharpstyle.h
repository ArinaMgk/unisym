
#ifndef _INC_INTEGER_STYLE_CSharp
#define _INC_INTEGER_STYLE_CSharp


// #define Main main

#define System uni
#if defined(_INC_CPP)
// an alternative to Microsoft C++ CLI

#undef  _OPT_CHARSET_UTF16
#define _OPT_CHARSET_UTF16
#include "../../cpp/integer"
#include "../../cpp/string"
#include "../../c/multichar.h"


typedef System::String string;
namespace System {
	typedef unsigned short ushort;
	typedef unsigned int uint;
	typedef unsigned long long ulong;
	//
	typedef signed char SByte;
	typedef byte Byte;
	typedef Integer_T<int16> Int16;
	typedef Integer_T<int32> Int32;
	typedef Integer_T<int64> Int64;
	typedef Integer_T<uint16> Uint16;
	typedef Integer_T<uint32> Uint32;
	typedef Integer_T<uint64> Uint64;
	//
	typedef Integer_T<float32> Single;
	typedef Integer_T<float64> Double;
	//{TODO} Decimal
	typedef Integer_T<bool> Boolean;
	typedef Integer_T<unit_utf16_t> Char;

	template<typename type0>
	class Object : public uni::Property<Object<type0>> {
		//{TODO}
	};
	template<typename type0> using object = Object<type0>;

}

#endif
#endif

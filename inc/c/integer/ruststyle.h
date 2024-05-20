
#ifndef _INC_INTEGER_STYLE_RUST
#define _INC_INTEGER_STYLE_RUST

#include "../integer.h"

typedef uint8 u8;//[CONFLICT] u8 is short for uint8 or utf-8
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;
#define u128
typedef stdint isize;

typedef sint8 i8;
typedef sint16 i16;	
typedef sint32 i32;
typedef sint64 i64;
#define i128
typedef stduint usize;

typedef float  f32;
typedef double f64;

//{} char
//{} bool

#endif

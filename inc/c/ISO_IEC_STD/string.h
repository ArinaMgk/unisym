// ASCII C99 TAB4 CRLF
// Attribute: ArnCovenant
// LastCheck: RFZ11
// AllAuthor: @dosconio
// ModuTitle: Alias for ISO IEC Standard CPL string.h
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../ustring.h"

#ifdef __cplusplus
extern "C" {
#endif

#define memset MemSet

void* memmove(void* dest, const void* src, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
int memcmp(const void* lhs, const void* rhs, size_t count);
void* memchr(const void* ptr, int ch, size_t count);
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t count);
char* strcat(char* dest, const char* src);
int strcmp(const char* lhs, const char* rhs);
int strncmp(const char* lhs, const char* rhs, size_t count);
char* strchr(const char* str, int ch);
char* strrchr(const char* str, int ch);
char* strstr(const char* haystack, const char* needle);

#ifdef __cplusplus
}
#endif

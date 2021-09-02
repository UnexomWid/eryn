#ifndef STR_HXX_GUARD
#define STR_HXX_GUARD

#include <cstddef>

namespace str {
char* dup(const char* str);
char* dup(const char* str, size_t size);

unsigned char to_upper(unsigned char c);
bool          is_print(unsigned char c);
bool          is_blank(unsigned char c);
bool          valid_in_token(unsigned char c);
}

#endif
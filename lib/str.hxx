#ifndef STR_HXX_GUARD
#define STR_HXX_GUARD

#include <cstddef>

char* strDup(const char* str);
char* strDup(const char* str, size_t size);

unsigned char toUpper(unsigned char c);
bool isPrint(unsigned char c);
bool isBlank(unsigned char c);

#endif
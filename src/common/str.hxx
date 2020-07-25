#ifndef ERYN_COMMON_STR_HXX_GUARD
#define ERYN_COMMON_STR_HXX_GUARD

#include <cstddef>

char* strDup(const char* str);
char* strDup(const char* str, size_t size);

unsigned char toUpper(unsigned char c);
bool isPrint(unsigned char c);

#endif
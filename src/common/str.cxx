#include "str.hxx"

#include <cstring>

#include "../../lib/remem.hxx"

char* strDup(const char* str) {
    size_t length = strlen(str) + 1;
    char* ptr = reinterpret_cast<char*>(re::malloc(length, "StrDup String", __FILE__, __LINE__));

    memcpy(ptr, str, length);
    return ptr;
}

char* strDup(const char* str, size_t size) {
    char* ptr = reinterpret_cast<char*>(re::malloc(size + 1, "StrDup String", __FILE__, __LINE__));

    memcpy(ptr, str, size);
    ptr[size] = '\0';

    return ptr;
}

unsigned char toUpper(unsigned char c) {
    return c >= 'a' && c <= 'z' ? c - 32 : c;
}

bool isPrint(unsigned char c) {
    return (c >= ' ' && c <= '~');
}
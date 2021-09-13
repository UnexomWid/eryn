#include "str.hxx"

#include <cstring>

#include "remem.hxx"

char* str::dup(const char* str) {
    size_t length = strlen(str) + 1;
    char* ptr = reinterpret_cast<char*>(REMEM_MALLOC(length, "StrDup String"));

    memcpy(ptr, str, length);
    return ptr;
}

char* str::dup(const char* str, size_t size) {
    char* ptr = reinterpret_cast<char*>(REMEM_MALLOC(size + 1, "StrDup String"));

    memcpy(ptr, str, size);
    ptr[size] = '\0';

    return ptr;
}

unsigned char str::to_upper(unsigned char c) {
    return c >= 'a' && c <= 'z' ? c - 32 : c;
}

bool str::is_print(unsigned char c) {
    return (c >= ' ' && c <= '~');
}

bool str::is_blank(unsigned char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

bool str::valid_in_token(uint8_t c) {
    if(c >= 'A' && c <= 'Z')
        return true;
    if(c >= 'a' && c <= 'z')
        return true;
    if(c >= '0' && c <= '9')
        return true;
    if(c == '_' || c == '$')
        return true;
    return false;
}
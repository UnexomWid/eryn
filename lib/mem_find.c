#include "mem_find.h"

#include <string.h>

size_t mem_find(const uint8_t* source, size_t sourceSize, const uint8_t* pattern, uint8_t patternSize, const uint8_t* lookup) {
    if(patternSize < HORSPOOL_THRESHOLD)
        return mem_find_kmp(source, sourceSize, pattern, patternSize, lookup);
    return mem_find_horspool(source, sourceSize, pattern, patternSize, lookup);
}

size_t mem_find_horspool(const uint8_t* source, size_t sourceSize, const uint8_t* pattern, uint8_t patternSize, const uint8_t* lookup) {
    const uint8_t* limit = source + sourceSize - patternSize + 1;
    size_t index = 0;
    size_t offset;

    while(source < limit) {
        if(membrcmp(source, pattern, patternSize))
            return index;

        offset = lookup[*(source + patternSize - 1)];
        index += offset;
        source += offset;
    }

    return sourceSize;
}

size_t mem_find_kmp(const uint8_t* source, size_t sourceSize, const uint8_t* pattern, uint8_t patternSize, const uint8_t* lookup) {
    size_t i = 0;
    size_t j = 0;

    while (i < sourceSize) {
        if(pattern[j] == source[i]) {
            ++j;
            ++i;
        }

        if(j == patternSize)
            return i - j;
            
        if(i < sourceSize && pattern[j] != source[i]) {
            if(j != 0)
                j = lookup[j - 1];
            else i = i + 1;
        }
    }

    return sourceSize;
}

void build_horspool_lookup(uint8_t* lookup, const uint8_t* pattern, uint8_t patternSize) {
    const uint8_t* limit = pattern + patternSize - 1;
    uint8_t value = patternSize - 1;

    memset(lookup, patternSize, 256);

    while(pattern < limit) {
        lookup[*pattern] = value--;
        ++pattern;
    }

    lookup[*pattern] = patternSize;
}

void build_kmp_lookup(uint8_t* lookup, const uint8_t* pattern, uint8_t patternSize) {
    uint8_t len = 0;
    uint8_t i = 1;

    lookup[0] = 0;

    while (i < patternSize) {
        if (pattern[i] == pattern[len])
            lookup[i++] = ++len;
        else if (len != 0)
            len = lookup[len - 1];
        else lookup[i++] = 0;
    }
}

bool membcmp(const uint8_t* m1, const uint8_t* m2, size_t length) {
    const uint8_t* m1Limit = m1 + length;

    while(*m1 == *m2 && m1 < m1Limit) {
        ++m1;
        ++m2;
    }

    return m1 == m1Limit;
}

bool membrcmp(const uint8_t* m1, const uint8_t* m2, size_t length) {
    const uint8_t* m1Limit = m1;

    m1 += length - 1;
    m2 += length - 1;

    while(*m1 == *m2 && m1 >= m1Limit) {
        --m1;
        --m2;
    }

    return m1 < m1Limit;
}
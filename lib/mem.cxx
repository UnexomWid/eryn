#include "mem.hxx"

#include <cstring>
#include <cstdlib>

const uint8_t* mem::find(const void* source, size_t sourceSize, const void* pattern, size_t patternSize) {
    auto src = static_cast<const uint8_t*>(source);
    auto ptn = static_cast<const uint8_t*>(pattern);

    auto ptr = src;
    auto limit = src + sourceSize;

    while(ptr < limit) {
        auto pos = static_cast<const uint8_t*>(memchr(ptr, *ptn, limit - ptr));

        // Not found or pattern too big.
        if(pos == nullptr || (pos + patternSize > limit)) {
            return src + sourceSize;
        }

        // First char was already found, compare the rest.
        if(mem::cmp(ptn + 1, pos + 1, patternSize - 1)) {
            return pos;
        }

        ptr = pos + 1;
    }

    return src + sourceSize;
}

bool mem::cmp(const uint8_t* m1, const uint8_t* m2, size_t size) {
    return 0 == memcmp(m1, m2, size);
}

bool mem::rcmp(const uint8_t* m1, const uint8_t* m2, size_t length) {
    const uint8_t* m1Limit = m1;

    m1 += length - 1;
    m2 += length - 1;

    while(*m1 == *m2 && m1 >= m1Limit) {
        --m1;
        --m2;
    }

    return m1 < m1Limit;
}
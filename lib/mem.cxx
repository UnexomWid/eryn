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
        // Note that mem::cmp with a size of 0 returns false, hence the check for the patternSize.
        if(patternSize == 1 || mem::cmp(ptn + 1, pos + 1, patternSize - 1)) {
            return pos;
        }

        ptr = pos + 1;
    }

    return src + sourceSize;
}

bool mem::cmp(const void* m1, const void* m2, size_t size) {
    if(size == 0) {
        return false;
    }

    return 0 == memcmp(m1, m2, size);
}

bool mem::cmp(const void* m1, const std::string& m2) {
    return mem::cmp(m1, m2.c_str(), m2.size());
}

bool mem::rcmp(const void* m1, const void* m2, size_t size) {
    if(size == 0) {
        return false;
    }
    
    auto left = static_cast<const uint8_t*>(m1);
    auto right = static_cast<const uint8_t*>(m2);

    const uint8_t* m1Limit = left;

    left += size - 1;
    right += size - 1;

    while(*left == *right && left >= m1Limit) {
        --left;
        --right;
    }

    return m1 < m1Limit;
}

bool mem::rcmp(const void* m1, const std::string& m2) {
    return mem::rcmp(m1, m2.c_str(), m2.size());
}
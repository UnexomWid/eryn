#ifndef MEM_FIND_H_GUARD
#define MEM_FIND_H_GUARD

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HORSPOOL_THRESHOLD 3

namespace mem {
    const uint8_t* find(const void* source, size_t sourceSize, const void* pattern, size_t patternSize);
    bool cmp(const uint8_t* m1, const uint8_t* m2, size_t length);
    bool rcmp(const uint8_t* m1, const uint8_t* m2, size_t length);
}

#endif
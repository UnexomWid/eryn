#ifndef MEM_FIND_H_GUARD
#define MEM_FIND_H_GUARD

#include <cstddef>
#include <cstdint>
#include <cstdbool>
#include <string>

#define HORSPOOL_THRESHOLD 3

namespace mem {
    const uint8_t* find(const void* source, size_t sourceSize, const void* pattern, size_t patternSize);
    bool cmp(const void* m1, const void* m2, size_t size);
    bool cmp(const void* m1, const std::string& m2);
    bool rcmp(const void* m1, const void* m2, size_t size);
    bool rcmp(const void* m1, const std::string& m2);
}

#endif
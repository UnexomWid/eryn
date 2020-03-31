#ifndef MEM_INDEX_H_GUARD
#define MEM_INDEX_H_GUARD

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Gets the line and character from an index. Counts LF, ignores CR.
void mem_lnchr(const uint8_t* source, size_t index, size_t* ln, size_t* chr);

#ifdef __cplusplus
}
#endif
#ifndef MEM_INDEX_H_GUARD
#define MEM_INDEX_H_GUARD

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Gets the line and column from an index. Counts LF, ignores CR.
void mem_lncol(const uint8_t* source, size_t index, size_t* ln, size_t* col);

/// Gets a chunk of memory at an index. The total chunk size is max 2 * chunk + 1. The index position in the chunk is placed in chunkIndex (if pointer is not 0).
uint8_t* mem_chunk(const uint8_t* source, register size_t index, size_t size, register size_t chunk, size_t* chunkIndex, size_t* chunkSize);

/// Same as mem_chunk, stops at LF and CR.
uint8_t* mem_lnchunk(const uint8_t* source, size_t index, size_t size, register size_t chunk, size_t* chunkIndex, size_t* chunkSize);

#ifdef __cplusplus
}
#endif

#endif
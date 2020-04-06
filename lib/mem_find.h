#ifndef MEM_FIND_H_GUARD
#define MEM_FIND_H_GUARD

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define HORSPOOL_THRESHOLD 3

#ifdef __cplusplus
extern "C" {
#endif

/// If patternSize < HORSPOOL_THRESHOLD , uses KMP. Otherwise, uses Horspool. Assumes that the correct lookup is provided (i.e. Horspool for Horspool, KMP for KMP).
size_t mem_find(const uint8_t* source, size_t sourceSize, const uint8_t* pattern, uint8_t patternSize, const uint8_t* lookup);

/// Returns the index of the first occurrence of a pattern in a source, by using the Boyer-Moore-Horspool algorithm. If it doesn't appear, returns the source size.
size_t mem_find_horspool(const uint8_t* source, size_t sourceSize, const uint8_t* pattern, uint8_t patternSize, const uint8_t* lookup);

/// Returns the index of the first occurrence of a pattern in a source, by using the Knuth-Morris-Pratt algorithm. If it doesn't appear, returns the source size.
size_t mem_find_kmp(const uint8_t* source, size_t sourceSize, const uint8_t* pattern, uint8_t patternSize, const uint8_t* lookup);

/// Builds the Horspool lookup for a pattern. The size of the lookup is 256. Use this with mem_find_horspool (and mem_find if patternSize < HORSPOOL_THRESHOLD).
void build_horspool_lookup(uint8_t* lookup, const uint8_t* pattern, uint8_t patternSize);

/// Builds the KMP lookup for a pattern. The size of the lookup is equal to patternSize. Use this with mem_find_kmp (and mem_find if patternSize >= HORSPOOL_THRESHOLD).
void build_kmp_lookup(uint8_t* lookup, const uint8_t* pattern, uint8_t patternSize);

/// Compares the first 'length' bytes of 2 memory blocks. Returns true if they match, false otherwise.
bool membcmp(const uint8_t* m1, const uint8_t* m2, size_t length);

/// Compares the first 'length' bytes of 2 memory blocks, in reverse. Returns true if they match, false otherwise.
bool membrcmp(const uint8_t* m1, const uint8_t* m2, size_t length);

#ifdef __cplusplus
}
#endif

#endif
#include "mem_index.h"

#include <stdlib.h>

void mem_lncol(const uint8_t* source, size_t index, size_t* ln, size_t* col) {
    register size_t  l = 1;
    register size_t  c = 1;
    register uint8_t current;

    const uint8_t* limit = source + index;

    while(source < limit) {
        current = *(source++);

        if(current == '\n') {
            ++l;
            c = 1;
        } else if(current != '\r') {
            ++c;
        }
    }

    *ln = l;
    *col = c;
}

uint8_t* mem_chunk(const uint8_t* source, register size_t index, size_t size, register size_t chunk, size_t* chunkIndex) {
    register size_t startIndex;
    register size_t endIndex;

    if(index >= chunk)
        startIndex = index - chunk;
    else startIndex = 0;

    if(chunkIndex != 0)
        *chunkIndex = index - startIndex;

    endIndex = index + chunk;
    
    if(endIndex > size)
        endIndex = size;

    // Re-use this variable instead of creating a new one.
    chunk = (endIndex - startIndex + 1);

    uint8_t* data = (uint8_t*) malloc(chunk);
    memcpy(data, source + startIndex, chunk);

    return data;
}

uint8_t* mem_lnchunk(const uint8_t* source, size_t index, size_t size, register size_t chunk, size_t* chunkIndex) {
    register const uint8_t* start = source + index;
    register const uint8_t* end = source + index;

    register const uint8_t* limit;

    if(index >= chunk)
        limit = source + index - chunk;
    else limit = source;

    while(start > limit && *start != '\n' && *start != '\r')
        --start;

    if(chunkIndex != 0)
        *chunkIndex = source + index - limit;

    if(index + chunk <= size)
        limit = source + index + chunk;
    else limit = source + size;

    while(end < limit && *end != '\n' && *end != '\r')
        ++end;

    // Re-use this variable instead of creating a new one.
    chunk = (end - start + 1);

    uint8_t* data = (uint8_t*) malloc(chunk);
    memcpy(data, start, chunk);

    return data;
}
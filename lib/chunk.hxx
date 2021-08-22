#ifndef CHUNK_HXX_GUARD
#define CHUNK_HXX_GUARD

#include <cstddef>
#include <cstdint>
#include <string>

struct Chunk {
    std::string data;
    size_t index;
    size_t line;
    size_t column;

    Chunk();
    Chunk(const uint8_t* source, size_t sourceSize, size_t index, size_t maxChunkSize);
};

#endif
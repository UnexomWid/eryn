#include "chunk.hxx"

Chunk::Chunk() : data(""), index(0), line(0), column(0) { }

Chunk::Chunk(const uint8_t* source, size_t sourceSize, size_t index, size_t maxChunkSize) {
    this->line = 1;
    this->column = 1;

    uint8_t current;

    const uint8_t* limit = source + index;
    const uint8_t* ptr = source;

    while(ptr < limit) {
        current = *(ptr++);

        if(current == '\n') {
            ++this->line;
            this->column = 1;

            // Pointer was already incremented. Check for \r and ignore it,
            // such that this can work with \r, \n, \r\n, and \n\r
            if(ptr < limit && *ptr == '\r') {
                ++ptr;
            }
        } else if(current == '\r') {
            ++this->line;
            this->column = 1;

            // Same as above.
            if(ptr < limit && *ptr == '\n') {
                ++ptr;
            }
        } else {
            ++this->column;
        }
    }

    auto start         = &source[index];
    auto end           = &source[index];
    auto halfChunkSize = maxChunkSize / 2;

    if(index > halfChunkSize) {
        limit = source + index - halfChunkSize;
    } else {
        limit = source;
    }

    // Keep going left until either the limit or a new line is reached.
    while(start > limit && *start != '\n' && *start != '\r') {
        --start;
    }

    // If a new line is reached, the start ptr needs to be incremented
    // such that the chunk doesn't contain the new line character.
    if(*start == '\n' || *start == '\r') {
        ++start;
    }

    this->index = source + index - start;

    if(index + halfChunkSize < sourceSize) {
        limit = source + index + halfChunkSize;
    } else {
        limit = source + sourceSize - 1;
    }

    // Keep going right until either the limit or a new line is reached.
    while(end < limit && *end != '\n' && *end != '\r') {
        ++end;
    }

    // If a new line is reached, the end ptr needs to be decremented
    // such that the chunk doesn't contain the new line character.
    if(*end == '\n' || *end == '\r') {
        --end;
    }

    // The size can be smaller than chunkSize if the source is small enough.
    auto finalSize = (end - start + 1);
    this->data = std::string(reinterpret_cast<const char*>(start), finalSize);
}
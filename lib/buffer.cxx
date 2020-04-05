/* Provides simple C-style memory management for byte buffers which may change size.
 * The allocations and expansions are made in a quadratic manner (i.e. powers of 2).
 */
#include "buffer.hxx"

#include <string>
#include <cstdlib>
#include <cstring>

/// Brings the size to the closest power of 2 >= size.
void adjustBufferSize(size_t &size) {
    --size;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;

    if constexpr(sizeof(size_t) == 8)
        size |= size >> 32;
    ++size;
}

uint8_t* qalloc(size_t &size) {
    adjustBufferSize(size);
    uint8_t* ptr = (uint8_t*) malloc(size);

    if(!ptr)
        throw MemoryException("Cannot allocate memory", size);
    printf("Allocated %p\n", ptr);
    return ptr;
}

uint8_t* qmalloc(size_t &size) {
    uint8_t* ptr = (uint8_t*) malloc(size);

    if(!ptr)
        throw MemoryException("Cannot allocate memory", size);
    printf("Allocated %p\n", ptr);
    return ptr;
}

uint8_t* qrealloc(uint8_t* buffer, size_t size) {
    uint8_t* ptr = (uint8_t*) realloc(buffer, size);

    if(!ptr)
        throw MemoryException("Cannot reallocate memory", size);
    printf("Reallocated %p\n", ptr);
    return ptr;
}

uint8_t* qexpand(uint8_t* buffer, size_t &size) {
    return qrealloc(buffer, (size *= 2));
}

void qfree(uint8_t* buffer) {
    if(buffer != nullptr && buffer != 0)
        free(buffer);
    printf("Freed %p\n", buffer);
}

MemoryException::MemoryException(const MemoryException &e) {
    message = strdup(e.message);
}

MemoryException::MemoryException(MemoryException &&e) {
    message = e.message;
    e.message = nullptr;
}

MemoryException::~MemoryException() {
    if(message != nullptr)
        free((char*) message);
}

MemoryException::MemoryException(const char* msg, size_t index) {
    std::string buffer(msg);

    buffer += "(";
    buffer += std::to_string(index);
    buffer += " bytes)";

    message = strdup(buffer.c_str());
}

const char* MemoryException::what() const {
    return message;
}

MemoryException& MemoryException::operator=(const MemoryException &e) {
    if(this == &e)
        return *this;

    if(message != nullptr)
        free((char*) message);

    message = strdup(e.message);

    return *this;
}
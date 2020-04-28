/* Provides simple C-style memory management for byte buffers which may change size.
 * The allocations and expansions are made in a quadratic manner (i.e. powers of 2).
 * Also contains other misc functions, like qstrdup, which use these memory management functions.
 */
#include "buffer.hxx"

#include <string>
#include <cstdlib>
#include <cstring>

#if defined(LOG_MEMORY_OPERATIONS) || defined (LOG_MEMORY_FULLY_FREED)
    #define LOG_MEM(...) printf("[memory] " __VA_ARGS__);
    size_t allocatedAmount = 0;
#endif

#ifdef _MSC_VER
    #pragma warning(disable: 4996 4293)
#endif

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

    #ifdef LOG_MEMORY_OPERATIONS
        LOG_MEM("Allocated %p\n", ptr);
    #endif

    #if defined(LOG_MEMORY_OPERATIONS) || defined(LOG_MEMORY_FULLY_FREED)
        ++allocatedAmount;
    #endif

    return ptr;
}

uint8_t* qmalloc(size_t size) {
    uint8_t* ptr = (uint8_t*) malloc(size);

    if(!ptr)
        throw MemoryException("Cannot allocate memory", size);

    #ifdef LOG_MEMORY_OPERATIONS
        LOG_MEM("Allocated %p\n", ptr);
    #endif

    #if defined(LOG_MEMORY_OPERATIONS) || defined(LOG_MEMORY_FULLY_FREED)
        ++allocatedAmount;
    #endif

    return ptr;
}

uint8_t* qrealloc(uint8_t* buffer, size_t size) {
    uint8_t* ptr = (uint8_t*) realloc(buffer, size);

    if(!ptr)
        throw MemoryException("Cannot reallocate memory", size);

    #ifdef LOG_MEMORY_OPERATIONS
        LOG_MEM("Reallocated %p -> %p\n", buffer, ptr);
    #endif

    return ptr;
}

uint8_t* qexpand(uint8_t* buffer, size_t &size) {
    return qrealloc(buffer, (size *= 2));
}

void qfree(void* buffer) noexcept {
    if(buffer != nullptr && buffer != 0)
        free(buffer);

    #if defined(LOG_MEMORY_OPERATIONS) || defined(LOG_MEMORY_FULLY_FREED)
        --allocatedAmount;
    #endif

    #ifdef LOG_MEMORY_OPERATIONS
        LOG_MEM("Freed %p (%zu remaining)\n", buffer, allocatedAmount);
    #endif

    #ifdef LOG_MEMORY_FULLY_FREED
        if(allocatedAmount == 0)
            LOG_MEM("All allocated memory blocks have been freed\n");
    #endif
}

char* qstrdup(const char* str) {
    size_t length = strlen(str) + 1;
    char* ptr = reinterpret_cast<char*>(qmalloc(length));

    memcpy(ptr, str, length);
    return ptr;
}

char* qstrndup(const char* str, size_t size) {
    char* ptr = reinterpret_cast<char*>(qmalloc(size + 1));

    memcpy(ptr, str, size);
    ptr[size] = '\0';

    return ptr;
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

    buffer += " (";
    buffer += std::to_string(index);
    buffer += " bytes)";

    message = strdup(buffer.c_str());
}

const char* MemoryException::what() const noexcept {
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

BinaryData::BinaryData() : BinaryData(nullptr, 0) { }
BinaryData::BinaryData(const uint8_t* d, size_t s) : data(d), size(s) { }
BinaryData::BinaryData(const BinaryData& binaryData) : data(binaryData.data), size(binaryData.size) { }
BinaryData::BinaryData(const BinaryData&& binaryData) : data(binaryData.data), size(binaryData.size) { }

BinaryData BinaryData::copy() {
    uint8_t* dataCopy = qmalloc(size);
    memcpy(dataCopy, data, size);

    return BinaryData(dataCopy, size);
}

BinaryData& BinaryData::operator=(const BinaryData& binaryData) {
    if(&binaryData == this)
        return *this;
    data = binaryData.data;
    size = binaryData.size;

    return *this;
}

#if defined(LOG_MEMORY_OPERATIONS) || defined (LOG_MEMORY_FULLY_FREED)
    #undef LOG_MEM
#endif
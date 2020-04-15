/* Provides simple C-style memory management for byte buffers which may change size.
 * The allocations and expansions are made in a quadratic manner (i.e. powers of 2).
 */
#include "buffer.hxx"

#include <string>
#include <cstdlib>
#include <cstring>

#ifdef LOG_MEMORY_ALLOCATIONS
    #define LOG_MEM(...) printf("\n[memory] "##__VA_ARGS__);
    size_t allocatedAmount = 0;
#else
    #define LOG_MEM(...)
#endif

#ifdef _MSC_VER
    #pragma warning(disable: 4996)
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

    #ifdef LOG_MEMORY_ALLOCATIONS
        LOG_MEM("Allocated %p", ptr);
        ++allocatedAmount;
    #endif

    return ptr;
}

uint8_t* qmalloc(size_t size) {
    uint8_t* ptr = (uint8_t*) malloc(size);

    if(!ptr)
        throw MemoryException("Cannot allocate memory", size);

    #ifdef LOG_MEMORY_ALLOCATIONS
        LOG_MEM("Allocated %p", ptr);
        ++allocatedAmount;
    #endif

    return ptr;
}

uint8_t* qrealloc(uint8_t* buffer, size_t size) {
    uint8_t* ptr = (uint8_t*) realloc(buffer, size);

    if(!ptr)
        throw MemoryException("Cannot reallocate memory", size);

    LOG_MEM("Reallocated %p -> %p", buffer, ptr);

    return ptr;
}

uint8_t* qexpand(uint8_t* buffer, size_t &size) {
    return qrealloc(buffer, (size *= 2));
}

void qfree(void* buffer) {
    if(buffer != nullptr && buffer != 0)
        free(buffer);

    #ifdef LOG_MEMORY_ALLOCATIONS
        --allocatedAmount;

        LOG_MEM("Freed %p (%zu remaining)", buffer, allocatedAmount);
        if(allocatedAmount == 0)
            LOG_MEM("All allocated memory blocks have been freed");
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

#undef LOG_MEM
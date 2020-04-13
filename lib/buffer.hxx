/* Provides simple C-style memory management for byte buffers which may change size.
 * The allocations and expansions are made in a quadratic manner (i.e. powers of 2).
 * Also contains other misc functions, like qstrdup, which use these memory management functions.
 */
#ifndef BUFFER_HXX_GUARD
#define BUFFER_HXX_GUARD

#include <cstdint>
#include <exception>

/// Classic malloc. Rounds the size to a power of 2. Throws an exception if it fails.
uint8_t* qalloc(size_t &size);
/// Classic malloc. Leaves the size as-is. Throws an exception if it fails.
uint8_t* qmalloc(size_t size);
/// Classic realloc. Throws an exception if it fails.
uint8_t* qrealloc(uint8_t* buffer, size_t size);
/// Classic realloc with double the buffer size. Throws an exception if it fails.
uint8_t* qexpand(uint8_t* buffer, size_t &size);
/// Classic free with checks.
void qfree(void* buffer);

/// Classic strdup. Uses qmalloc behind the scenes. You should free the memory with qfree.
char* qstrdup(const char* str);

/// Occurs when (re)allocating memory.
class MemoryException : public std::exception {
    private:
        const char* message;
    public:
        MemoryException(MemoryException &&e);
        MemoryException(const MemoryException &e);
        MemoryException(const char* msg, size_t size);
        ~MemoryException();

        const char* what() const override;

        MemoryException& operator=(const MemoryException &e);
};

/// Simple struct for holding binary data.
struct BinaryData {
    const uint8_t* data;
    const size_t   size;

    BinaryData();
    BinaryData(const uint8_t* d, const size_t s);
    BinaryData(const BinaryData& binaryData);
    BinaryData(const BinaryData&& binaryData);

    BinaryData copy();
};

#endif
#ifndef BUFFER_HXX_GUARD
#define BUFFER_HXX_GUARD

#include <cstdint>

#include "bdp.hxx"

struct Buffer;
struct ConstBuffer;

struct Buffer {
    uint8_t* data;
    size_t size;
    size_t capacity;

    Buffer();
    Buffer(uint8_t* data, size_t size);
    Buffer(const Buffer& buffer);
    Buffer(Buffer&& buffer);

    ~Buffer();

    uint8_t* end() const noexcept;

    Buffer& operator=(const Buffer& buffer);
    Buffer& operator=(Buffer&& buffer);

    void write(uint8_t byte);
    void write(const uint8_t* bytes, size_t amount);
    void write_at(size_t index, const uint8_t* bytes, size_t amount);
    void write_bdp_name(const BDP::Header& header, const uint8_t* name, size_t nameSize);
    void write_bdp_value(const BDP::Header& header, const uint8_t* value, size_t valueSize);
    void write_bdp_pair(const BDP::Header& header, const uint8_t* name, size_t nameSize, const uint8_t* value, size_t valueSize);
    void write_length(size_t source, uint8_t count);
    void write_length(size_t index, size_t source, uint8_t count);
    void repeat(uint8_t byte, size_t amount);
    void move_right(size_t index, size_t count);
    void reserve(size_t amount);

    // Returns a pointer to the data and resets the buffer data pointer, along with the size and capacity.
    uint8_t* release();
    // Creates a ConstBuffer, shrinks the allocated memory for the data buffer if possible, and releases the data pointer.
    ConstBuffer finalize();
};

struct ConstBuffer {
    const uint8_t* data;
    size_t size;

    ConstBuffer(const void* data, size_t size);

    const uint8_t* end() const noexcept;

    const uint8_t* find(const std::string& pattern) const noexcept;
    const uint8_t* find(size_t index, const std::string& pattern) const noexcept;
    const uint8_t* find(const void* pattern, size_t patternSize) const noexcept;
    const uint8_t* find(size_t index, const void* pattern, size_t patternSize) const noexcept;

    size_t find_index(const std::string& pattern) const noexcept;
    size_t find_index(size_t index, const std::string& pattern) const noexcept;
    size_t find_index(const void* pattern, size_t patternSize) const noexcept;
    size_t find_index(size_t index, const void* pattern, size_t patternSize) const noexcept;

    bool match(const std::string& pattern) const noexcept;
    bool match(size_t index, const std::string& pattern) const noexcept;
    bool match(const void* pattern, size_t patternSize) const noexcept;
    bool match(size_t index, const void* pattern, size_t patternSize) const noexcept;
};

#endif
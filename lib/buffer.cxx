#include "buffer.hxx"

#include <string>
#include <cstdlib>
#include <cstring>

#include "mem.hxx"
#include "remem.hxx"

#ifdef _MSC_VER
    #pragma warning(disable: 4996 4293)
#endif

static constexpr size_t BUFFER_INITIAL_SIZE = 64;

Buffer::Buffer() : Buffer(nullptr, 0) {
    data = nullptr;
    size = 0;
    capacity = 0;
}

Buffer::Buffer(uint8_t* data, size_t size) :
    data(data), size(size), capacity(size) { }

Buffer::Buffer(const Buffer& buffer) :
    data(buffer.data), size(buffer.size), capacity(buffer.capacity) { }

Buffer::Buffer(Buffer&& buffer) {
    size     = buffer.size;
    capacity = buffer.capacity;
    data     = buffer.release();
}

Buffer::~Buffer() {
    if(data != nullptr) {
        REMEM_FREE(data);
    }
}

uint8_t* Buffer::end() const noexcept {
    return data + size;
}

Buffer& Buffer::operator=(const Buffer& buffer) {
    if(&buffer == this) {
        return *this;
    }
        
    size     = buffer.size;
    capacity = buffer.capacity;
    data     = buffer.data;

    return *this;
}

Buffer& Buffer::operator=(Buffer&& buffer) {
    if(&buffer == this) {
        return *this;
    }

    size     = buffer.size;
    capacity = buffer.capacity;
    data     = buffer.release();

    return *this;
}

void Buffer::clear() {
    size = 0;
}

void Buffer::write(uint8_t byte) {
    write(&byte, sizeof(uint8_t));
}

void Buffer::write(const uint8_t* bytes, size_t amount) {
    reserve(amount);
    memcpy(data + size, bytes, amount);
    size += amount;
}

void Buffer::write(const ConstBuffer& buffer) {
    write(buffer.data, buffer.size);
}

void Buffer::write_at(size_t index, const uint8_t* bytes, size_t amount) {
    auto diff = size - index;
    auto extra = diff < amount ? amount - diff : 0;

    reserve(extra);
    
    memcpy(data + index, bytes, amount);
    size += extra;
}

void Buffer::write_bdp_name(const BDP::Header& header, const uint8_t* name, size_t nameSize) {
    auto amount = nameSize + header.NAME_LENGTH_BYTE_SIZE;
    
    reserve(amount);
    BDP::writeName(&header, data + size, name, nameSize);

    size += amount;
}

void Buffer::write_bdp_value(const BDP::Header& header, const uint8_t* value, size_t valueSize) {
    auto amount = valueSize + header.VALUE_LENGTH_BYTE_SIZE;
    
    reserve(amount);
    BDP::writeValue(&header, data + size, value, valueSize);

    size += amount;
}

void Buffer::write_bdp_pair(const BDP::Header& header, const uint8_t* name, size_t nameSize, const uint8_t* value, size_t valueSize) {
    write_bdp_name(header, name, nameSize);
    write_bdp_value(header, value, valueSize);
}

void Buffer::write_length(size_t source, uint8_t count) {
    write_length(size, source, count);
}

void Buffer::write_length(size_t index, size_t source, uint8_t count) {
    auto diff = size - index;
    auto extra = diff < count ? count - diff : 0;
    
    reserve(extra);

    BDP::lengthToBytes(data + index, source, count);
    size += extra;
}

void Buffer::repeat(uint8_t byte, size_t amount) {
    reserve(amount);
    memset(end(), byte, amount);

    size += amount;
}

void Buffer::move_right(size_t index, size_t count) {
    reserve(count);

    memmove(data + index + count, data + index, size - index);
    size += count;
}

void Buffer::reserve(size_t amount) {
    if(amount == 0) {
        return;
    }

    if(data == nullptr) {
        capacity = BUFFER_INITIAL_SIZE;
        data = static_cast<uint8_t*>(REMEM_ALLOC(capacity, "Buffer"));
    }

    while(size + amount > capacity) {
        data = static_cast<uint8_t*>(REMEM_EXPAND(data, capacity));
    }
}

uint8_t* Buffer::release() {
    auto ptr = data;

    data = nullptr;
    size = 0;
    capacity = 0;

    return ptr;
}

ConstBuffer Buffer::finalize() {
    if(size != capacity) {
        data = (uint8_t*) REMEM_REALLOC(data, size);
    }

    ConstBuffer finalized(data, size);
    finalized.data = release();

    return finalized;
}

ConstBuffer::ConstBuffer() : data(nullptr), size(0) { }

ConstBuffer::ConstBuffer(const void* data, size_t size) :
    data(static_cast<const uint8_t*>(data)), size(size) { }

const uint8_t* ConstBuffer::end() const noexcept {
    return data + size;
}

const uint8_t* ConstBuffer::find(const std::string& pattern) const noexcept {
    return find(0, pattern.c_str(), pattern.size());
}

const uint8_t* ConstBuffer::find(size_t index, const std::string& pattern) const noexcept {
    return find(index, pattern.c_str(), pattern.size());
}

const uint8_t* ConstBuffer::find(const void* pattern, size_t patternSize) const noexcept {
    return find(0, pattern, patternSize);
}

const uint8_t* ConstBuffer::find(size_t index, const void* pattern, size_t patternSize) const noexcept {
    return mem::find(data + index, size - index, pattern, patternSize);
}

size_t ConstBuffer::find_index(const std::string& pattern) const noexcept {
    return find(pattern) - data;
}

size_t ConstBuffer::find_index(size_t index, const std::string& pattern) const noexcept {
    return find(index, pattern) - data;
}

size_t ConstBuffer::find_index(const void* pattern, size_t patternSize) const noexcept {
    return find(pattern, patternSize) - data;
}

size_t ConstBuffer::find_index(size_t index, const void* pattern, size_t patternSize) const noexcept {
    return find(index, pattern, patternSize) - data;
}

bool ConstBuffer::match(const std::string& pattern) const noexcept {
    return match(0, pattern.c_str(), pattern.size());
}

bool ConstBuffer::match(size_t index, const std::string& pattern) const noexcept {
    return match(index, pattern.c_str(), pattern.size());
}

bool ConstBuffer::match(const void* pattern, size_t patternSize) const noexcept {
    return match(0, pattern, patternSize);
}

bool ConstBuffer::match(size_t index, const void* pattern, size_t patternSize) const noexcept {
    auto len = size - index;
    auto min = len <= patternSize ? len : patternSize;

    return mem::cmp(data + index, pattern, min);
}

void ConstBuffer::finalize(ConstBuffer& buffer) {
    REMEM_FREE(const_cast<uint8_t*>(buffer.data));
}
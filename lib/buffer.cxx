#include "buffer.hxx"

#include <string>
#include <cstdlib>
#include <cstring>

#include "remem.hxx"

#ifdef _MSC_VER
    #pragma warning(disable: 4996 4293)
#endif

Buffer::Buffer() : Buffer(nullptr, 0) {
    data = nullptr;
    size = 0;
    capacity = 0;
}
Buffer::Buffer(const Buffer& Buffer) : data(Buffer.data), size(Buffer.size) { }
Buffer::Buffer(Buffer&& buffer) : data(buffer.data), size(buffer.size) {
    buffer = Buffer();
}

Buffer::~Buffer() {
    if(data != nullptr) {
        REMEM_FREE(data);
    }
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

void Buffer::write(const uint8_t* bytes, size_t amount) {
    reserve(size);
    memcpy(data + size, bytes, amount);
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
    BDP::writeName(&header, data + size, value, valueSize);

    size += amount;
}

void Buffer::write_bdp_pair(const BDP::Header& header, const uint8_t* name, size_t nameSize, const uint8_t* value, size_t valueSize) {
    write_bdp_name(header, name, nameSize);
    write_bdp_value(header, value, valueSize);
}

void Buffer::reserve(size_t amount) {
    while(size + amount > capacity) {
        data = static_cast<uint8_t*>(REMEM_EXPAND(data, size));
    }
}

uint8_t* Buffer::release() {
    auto ptr = data;

    data = nullptr;
    size = 0;
    capacity = 0;

    return ptr;
}
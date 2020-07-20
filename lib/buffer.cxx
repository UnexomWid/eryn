#include "buffer.hxx"

#include <string>
#include <cstdlib>
#include <cstring>

#ifdef _MSC_VER
    #pragma warning(disable: 4996 4293)
#endif

BinaryData::BinaryData() : BinaryData(nullptr, 0) { }
BinaryData::BinaryData(const uint8_t* d, size_t s) : data(d), size(s) { }
BinaryData::BinaryData(const BinaryData& binaryData) : data(binaryData.data), size(binaryData.size) { }
BinaryData::BinaryData(const BinaryData&& binaryData) : data(binaryData.data), size(binaryData.size) { }

BinaryData& BinaryData::operator=(const BinaryData& binaryData) {
    if(&binaryData == this)
        return *this;
    data = binaryData.data;
    size = binaryData.size;

    return *this;
}
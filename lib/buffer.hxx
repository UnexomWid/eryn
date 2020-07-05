#ifndef BUFFER_HXX_GUARD
#define BUFFER_HXX_GUARD

#include <cstdint>
#include <exception>

/// Simple struct for holding externally-managed binary data. This is NOT responsible for allocating or freeing memory.
struct BinaryData {
    const uint8_t* data;
    size_t size;

    BinaryData();
    BinaryData(const uint8_t* d, size_t s);
    BinaryData(const BinaryData& binaryData);
    BinaryData(const BinaryData&& binaryData);

    BinaryData& operator=(const BinaryData& binaryData);
};

#endif
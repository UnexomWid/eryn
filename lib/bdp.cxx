/**
 * BDP (https://github.com/UnexomWid/BDP)
 *
 * This project is licensed under the MIT license.
 * Copyright (c) 2019-2020 UnexomWid (https://uw.exom.dev)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bdp.hxx"

#include <memory>

/// The magic BDP value, which is located at the beginning of every BDP package.
const char* MAGIC_VALUE = "BDP";
/// The magic BDP value length.
const uint8_t MAGIC_VALUE_LENGTH = 3u;
/// The default size of the buffer used to copy data from one stream to another.
const size_t DEFAULT_BUFFER_SIZE = 16384u;

void checkByteSize(uint8_t byteSize) {
    if constexpr(sizeof(size_t) < 8)
        if(byteSize == 8)
            throw std::runtime_error("Cannot process 64-bit BDP packages with this version of the library; use the 64-bit version instead");
}

BDP::Header::Header(uint8_t nlbs, uint8_t vlbs) : NAME_MAX_LENGTH(getMaxLength(nlbs)),
                                                  VALUE_MAX_LENGTH(getMaxLength(vlbs)),
                                                  NAME_LENGTH_BIT_SIZE(nlbs),
                                                  VALUE_LENGTH_BIT_SIZE(vlbs),
                                                  NAME_LENGTH_BYTE_SIZE(nlbs / 8u),
                                                  VALUE_LENGTH_BYTE_SIZE(vlbs / 8u) { }

BDP::Header* BDP::writeHeader(std::ostream& output, uint8_t nameLengthBitSize, uint8_t valueLengthBitSize) {
    if (nameLengthBitSize != 8u && nameLengthBitSize != 16u && nameLengthBitSize != 32u && nameLengthBitSize != 64u)
        throw std::invalid_argument("nameLengthBitSize");
    if (valueLengthBitSize != 8u && valueLengthBitSize != 16u && valueLengthBitSize != 32u && valueLengthBitSize != 64u)
        throw std::invalid_argument("valueLengthBitSize");

    checkByteSize(nameLengthBitSize / 8);
    checkByteSize(valueLengthBitSize / 8);

    uint8_t lengthBitSizes = (((nameLengthBitSize) << 1u) | (valueLengthBitSize >> 3u));

    output.write(MAGIC_VALUE, MAGIC_VALUE_LENGTH);
    output.write((char*) (&lengthBitSizes), 1u);

    return new BDP::Header(nameLengthBitSize, valueLengthBitSize);
}

BDP::Header* BDP::writeHeader(uint8_t* output, uint8_t nameLengthBitSize, uint8_t valueLengthBitSize) {
    if (nameLengthBitSize != 8u && nameLengthBitSize != 16u && nameLengthBitSize != 32u && nameLengthBitSize != 64u)
        throw std::invalid_argument("nameLengthBitSize");
    if (valueLengthBitSize != 8u && valueLengthBitSize != 16u && valueLengthBitSize != 32u && valueLengthBitSize != 64u)
        throw std::invalid_argument("valueLengthBitSize");

    checkByteSize(nameLengthBitSize / 8);
    checkByteSize(valueLengthBitSize / 8);

    uint8_t lengthBitSizes = (((nameLengthBitSize) << 1u) | (valueLengthBitSize >> 3u));

    memcpy(output, MAGIC_VALUE, MAGIC_VALUE_LENGTH);
    memcpy(output + MAGIC_VALUE_LENGTH, &lengthBitSizes, 1u);

    return new BDP::Header(nameLengthBitSize, valueLengthBitSize);
}

size_t BDP::writeName(const BDP::Header* header, std::ostream& output, const uint8_t* name, size_t nameLength) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, nameLength);
}
size_t BDP::writeName(const BDP::Header* header, uint8_t* output, const uint8_t* name, size_t nameLength) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, nameLength);
}
size_t BDP::writeName(const BDP::Header* header, std::ostream& output, std::istream& name) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, DEFAULT_BUFFER_SIZE);
}
size_t BDP::writeName(const BDP::Header* header, std::ostream& output, std::istream& name, size_t bufferSize) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, bufferSize);
}
size_t BDP::writeName(const BDP::Header* header, uint8_t* output, std::istream& name) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, DEFAULT_BUFFER_SIZE);
}
size_t BDP::writeName(const BDP::Header* header, uint8_t* output, std::istream& name, size_t bufferSize) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, bufferSize);
}

size_t BDP::writeValue(const BDP::Header* header, std::ostream& output, const uint8_t* value, size_t valueLength) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, valueLength);
}
size_t BDP::writeValue(const BDP::Header* header, uint8_t* output, const uint8_t* value, size_t valueLength) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, valueLength);
}
size_t BDP::writeValue(const BDP::Header* header, std::ostream& output, std::istream& value) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, DEFAULT_BUFFER_SIZE);
}
size_t BDP::writeValue(const BDP::Header* header, std::ostream& output, std::istream& value, size_t bufferSize) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, bufferSize);
}
size_t BDP::writeValue(const BDP::Header* header, uint8_t* output, std::istream& value) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, DEFAULT_BUFFER_SIZE);
}
size_t BDP::writeValue(const BDP::Header* header, uint8_t* output, std::istream& value, size_t bufferSize) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, bufferSize);
}

size_t BDP::writePair(const BDP::Header* header, std::ostream& output, const uint8_t* name, size_t nameLength, const uint8_t* value, size_t valueLength) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output, value, valueLength);
}
size_t BDP::writePair(const BDP::Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, const uint8_t* value, size_t valueLength) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output + header->NAME_LENGTH_BYTE_SIZE + nameLength, value, valueLength);
}
size_t BDP::writePair(const BDP::Header* header, std::ostream& output, const uint8_t* name, size_t nameLength, std::istream& value) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output, value);
}
size_t BDP::writePair(const BDP::Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, std::istream& value) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output + header->NAME_LENGTH_BYTE_SIZE + nameLength, value);
}
size_t BDP::writePair(const BDP::Header* header, std::ostream& output, const uint8_t* name, size_t nameLength, std::istream& value, size_t bufferSize) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output, value, bufferSize);
}
size_t BDP::writePair(const BDP::Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, std::istream& value, size_t bufferSize) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output + header->NAME_LENGTH_BYTE_SIZE + nameLength, value, bufferSize);
}
size_t BDP::writePair(const BDP::Header* header, std::ostream& output, std::istream& name, const uint8_t* value, size_t valueLength) {
    return writeName(header, output, name) +
           writeValue(header, output, value, valueLength);
}
size_t BDP::writePair(const BDP::Header* header, uint8_t* output, std::istream& name, const uint8_t* value, size_t valueLength) {
    size_t count = writeName(header, output, name);
    return count + writeValue(header, output + count, value, valueLength);
}
size_t BDP::writePair(const BDP::Header* header, std::ostream& output, std::istream& name, const uint8_t* value, size_t valueLength, size_t bufferSize) {
    return writeName(header, output, name, bufferSize) +
           writeValue(header, output, value, valueLength);
}
size_t BDP::writePair(const BDP::Header* header, uint8_t* output, std::istream& name, const uint8_t* value, size_t valueLength, size_t bufferSize) {
    size_t count = writeName(header, output, name, bufferSize);
    return count + writeValue(header, output + count, value, valueLength);
}
size_t BDP::writePair(const BDP::Header* header, std::ostream& output, std::istream& name, std::istream& value) {
    return writeName(header, output, name) +
           writeValue(header, output, value);
}
size_t BDP::writePair(const BDP::Header* header, uint8_t* output, std::istream& name, std::istream& value) {
    size_t count = writeName(header, output, name);
    return count + writeValue(header, output + count, value);
}
size_t BDP::writePair(const BDP::Header* header, std::ostream& output, std::istream& name, std::istream& value, size_t bufferSize) {
    return writeName(header, output, name, bufferSize) +
           writeValue(header, output, value, bufferSize);
}
size_t BDP::writePair(const BDP::Header* header, uint8_t* output, std::istream& name, std::istream& value, size_t bufferSize) {
    size_t count = writeName(header, output, name, bufferSize);
    return count + writeValue(header, output + count, value, bufferSize);
}

size_t BDP::writeData(size_t maxLength, uint8_t lengthByteSize, std::ostream& output, const uint8_t* data, size_t dataLength) {
    checkByteSize(lengthByteSize);

    if(isLittleEndian()) {
        output.write(reinterpret_cast<char*>(&dataLength), lengthByteSize);
    } else {
        auto dataLengthBytes = std::make_unique<uint8_t[]>(lengthByteSize);
        reversedValueToBytes(dataLengthBytes.get(), dataLength, lengthByteSize);

        output.write((char*) (&dataLengthBytes[0]), lengthByteSize);
    }

    output.write((char*) (&data[0]), dataLength);

    return lengthByteSize + dataLength;
}
size_t BDP::writeData(size_t maxLength, uint8_t lengthByteSize, uint8_t* output, const uint8_t* data, size_t dataLength) {
    lengthToBytes(output, dataLength, lengthByteSize);

    memcpy(output + lengthByteSize, data, static_cast<size_t>(dataLength));

    return lengthByteSize + dataLength;
}
size_t BDP::writeData(size_t maxLength, uint8_t lengthByteSize, std::ostream& output, std::istream& data, size_t bufferSize) {
    checkByteSize(lengthByteSize);

    size_t inputLength = 0u;
    size_t diff;
    size_t nextLength;

    auto buffer = std::make_unique<char[]>(static_cast<size_t>(bufferSize));

    std::streampos lastPos = output.tellp();

    // Write a placeholder value, as the actual length is unknown yet.
    output.write(reinterpret_cast<char*>(&inputLength), lengthByteSize);

    while((diff = maxLength - inputLength) != 0u && !data.eof()) {
        nextLength = diff < bufferSize ? diff : bufferSize;
        data.read(buffer.get(), nextLength);
        output.write(buffer.get(), data.gcount());
        inputLength += static_cast<size_t>(data.gcount());
    }

    std::streampos endPos = output.tellp();
    output.seekp(lastPos);

    // Write the actual length.
    if(isLittleEndian()) {
        output.write(reinterpret_cast<char*>(&inputLength), lengthByteSize);
    } else {
        auto inputLengthBytes = std::make_unique<uint8_t[]>(lengthByteSize);
        reversedValueToBytes(inputLengthBytes.get(), inputLength, lengthByteSize);

        output.write((char*) (&inputLengthBytes[0]), lengthByteSize);
    }

    output.seekp(endPos);

    return lengthByteSize + inputLength;
}
size_t BDP::writeData(size_t maxLength, uint8_t lengthByteSize, uint8_t* output, std::istream& data, size_t bufferSize) {
    size_t inputLength = 0u;
    size_t index = 0;
    size_t diff;
    size_t nextLength;

    // Write a placeholder value, as the actual length is unknown.
    lengthToBytes(output, inputLength, lengthByteSize);

    index += lengthByteSize;

    while((diff = maxLength - inputLength) != 0u && !data.eof()) {
        nextLength = diff < bufferSize ? diff : bufferSize;
        data.read((char*) (output + index), nextLength);

        index += static_cast<size_t>(data.gcount());
        inputLength += static_cast<size_t>(data.gcount());
    }

    // Write the actual length.
    lengthToBytes(output, inputLength, lengthByteSize);

    return lengthByteSize + inputLength;
}

BDP::Header* BDP::readHeader(std::istream& input) {
    auto magic = std::make_unique<char[]>(MAGIC_VALUE_LENGTH + 1u);
    input.read(magic.get(), MAGIC_VALUE_LENGTH);

    magic[MAGIC_VALUE_LENGTH] = '\0';

    if(strcmp(magic.get(), MAGIC_VALUE) != 0)
        throw std::invalid_argument("input");

    uint8_t nameLengthBitSize = 64u;
    uint8_t valueLengthBitSize = 64u;

    auto lengthBitSizes = std::make_unique<uint8_t>();
    input.read((char*) lengthBitSizes.get(), 1u);

    for(uint8_t i = 7u; i >= 4u; --i, nameLengthBitSize >>= 1)
        if(*lengthBitSizes.get() & (1u << i))
            break;

    if(nameLengthBitSize == 0)
        throw std::invalid_argument("input: Invalid package header");

    for(uint8_t i = 3u; i >= 0u; --i, valueLengthBitSize >>= 1)
        if (*lengthBitSizes.get() & (1u << i))
            break;

    if(valueLengthBitSize == 0)
        throw std::invalid_argument("input: Invalid package header");

    checkByteSize(nameLengthBitSize / 8);
    checkByteSize(valueLengthBitSize / 8);

    return new BDP::Header(nameLengthBitSize, valueLengthBitSize);
}
BDP::Header* BDP::readHeader(const uint8_t* input) {
    size_t index = 0;

    auto magic = std::make_unique<char[]>(MAGIC_VALUE_LENGTH + 1u);
    memcpy(magic.get(), input, MAGIC_VALUE_LENGTH);

    index += MAGIC_VALUE_LENGTH;
    magic[MAGIC_VALUE_LENGTH] = '\0';

    if(strcmp(magic.get(), MAGIC_VALUE) != 0)
        throw std::invalid_argument("input");

    uint8_t nameLengthBitSize = 64u;
    uint8_t valueLengthBitSize = 64u;

    auto lengthBitSizes = std::make_unique<uint8_t>();
    memcpy(lengthBitSizes.get(), input + index, 1);
    ++index;

    for(uint8_t i = 7u; i >= 4u; --i, nameLengthBitSize >>= 1)
        if(*lengthBitSizes.get() & (1u << i))
            break;

    if(nameLengthBitSize == 0)
        throw std::invalid_argument("input: Invalid package header");

    for(uint8_t i = 3u; i >= 0u; --i, valueLengthBitSize >>= 1)
        if (*lengthBitSizes.get() & (1u << i))
            break;

    if(valueLengthBitSize == 0)
        throw std::invalid_argument("input: Invalid package header");

    checkByteSize(nameLengthBitSize / 8);
    checkByteSize(valueLengthBitSize / 8);

    return new BDP::Header(nameLengthBitSize, valueLengthBitSize);
}

size_t BDP::readName(const BDP::Header* header, std::istream& input, uint8_t* name, size_t* nameLength) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, nameLength);
}
size_t BDP::readName(const BDP::Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, nameLength);
}
size_t BDP::readName(const BDP::Header* header, std::istream& input, std::ostream& name) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, DEFAULT_BUFFER_SIZE);
}
size_t BDP::readName(const BDP::Header* header, std::istream& input, std::ostream& name, size_t bufferSize) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, bufferSize);
}
size_t BDP::readName(const BDP::Header* header, const uint8_t* input, std::ostream& name, size_t* nameLength) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, nameLength);
}

size_t BDP::readValue(const BDP::Header* header, std::istream& input, uint8_t* value, size_t* valueLength) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, valueLength);
}
size_t BDP::readValue(const BDP::Header* header, const uint8_t* input, uint8_t* value, size_t* valueLength) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, valueLength);
}
size_t BDP::readValue(const BDP::Header* header, std::istream& input, std::ostream& value) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, DEFAULT_BUFFER_SIZE);
}
size_t BDP::readValue(const BDP::Header* header, std::istream& input, std::ostream& value, size_t bufferSize) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, bufferSize);
}
size_t BDP::readValue(const BDP::Header* header, const uint8_t* input, std::ostream& value, size_t* valueLength) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, valueLength);
}

size_t BDP::readPair(const BDP::Header* header, std::istream& input, uint8_t* name, size_t* nameLength, uint8_t* value, size_t* valueLength) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value, valueLength);
}
size_t BDP::readPair(const BDP::Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength, uint8_t* value, size_t* valueLength) {
    return readName(header, input, name, nameLength) +
           readValue(header, input + header->NAME_LENGTH_BYTE_SIZE + *nameLength, value, valueLength);
}
size_t BDP::readPair(const BDP::Header* header, std::istream& input, uint8_t* name, size_t* nameLength, std::ostream& value) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value);
}
size_t BDP::readPair(const BDP::Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength, std::ostream& value, size_t* valueLength) {
    return readName(header, input, name, nameLength) +
           readValue(header, input + header->NAME_LENGTH_BYTE_SIZE + *nameLength, value, valueLength);
}
size_t BDP::readPair(const BDP::Header* header, std::istream& input, uint8_t* name, size_t* nameLength, std::ostream& value, size_t bufferSize) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value, bufferSize);
}
size_t BDP::readPair(const BDP::Header* header, std::istream& input, std::ostream& name, uint8_t* value, size_t* valueLength) {
    return readName(header, input, name) +
           readValue(header, input, value, valueLength);
}
size_t BDP::readPair(const BDP::Header* header, const uint8_t* input, std::ostream& name, size_t* nameLength, uint8_t* value, size_t* valueLength) {
    size_t count = readName(header, input, name, nameLength);
    return count + readValue(header, input + count, value, valueLength);
}
size_t BDP::readPair(const BDP::Header* header, std::istream& input, std::ostream& name, uint8_t* value, size_t* valueLength, size_t bufferSize) {
    return readName(header, input, name, bufferSize) +
           readValue(header, input, value, valueLength);
}
size_t BDP::readPair(const BDP::Header* header, std::istream& input, std::ostream& name, std::ostream& value) {
    return readName(header, input, name) +
           readValue(header, input, value);
}
size_t BDP::readPair(const BDP::Header* header, const uint8_t* input, std::ostream& name, size_t* nameLength, std::ostream& value, size_t* valueLength) {
    size_t count = readName(header, input, name, nameLength);
    return count + readValue(header, input + count, value, valueLength);
}
size_t BDP::readPair(const BDP::Header* header, std::istream& input, std::ostream& name, std::ostream& value, size_t bufferSize) {
    return readName(header, input, name, bufferSize) +
           readValue(header, input, value, bufferSize);
}

size_t BDP::readData(uint8_t lengthByteSize, std::istream& input, uint8_t*& output, size_t* outputLength) {
    checkByteSize(lengthByteSize);

    size_t length = 0u;

    if(isLittleEndian()) {
        input.read(reinterpret_cast<char*>(&length), lengthByteSize);
    } else {
        auto outputLengthBytes = std::make_unique<uint8_t[]>(lengthByteSize);
        input.read((char*) outputLengthBytes.get(), lengthByteSize);

        reversedBytesToValue(length, outputLengthBytes.get(), lengthByteSize);
    }

    input.read((char*) (&output[0]), length);

    if(outputLength != nullptr)
        *outputLength = length;

    return lengthByteSize + length;
}
size_t BDP::readData(uint8_t lengthByteSize, const uint8_t* input, uint8_t*& output, size_t* outputLength) {
    size_t length = 0u;

    bytesToLength(length, input, lengthByteSize);

    memcpy(output, input + lengthByteSize, static_cast<size_t>(length));

    if(outputLength != nullptr)
        *outputLength = length;

    return lengthByteSize + length;
}
size_t BDP::readData(uint8_t lengthByteSize, std::istream& input, std::ostream& output, size_t bufferSize) {
    checkByteSize(lengthByteSize);

    size_t length = 0u;

    if(isLittleEndian()) {
        input.read(reinterpret_cast<char*>(&length), lengthByteSize);
    } else {
        auto outputLengthBytes = std::make_unique<uint8_t[]>(lengthByteSize);
        input.read((char*) outputLengthBytes.get(), lengthByteSize);

        reversedBytesToValue(length, outputLengthBytes.get(), lengthByteSize);
    }

    auto buffer = std::make_unique<char[]>(static_cast<size_t>(bufferSize));
    size_t nextLength;

    while(length > 0u && !input.eof()) {
        nextLength = length < bufferSize ? length : bufferSize;
        input.read(buffer.get(), nextLength);
        output.write(buffer.get(), input.gcount());
        length -= static_cast<size_t>(input.gcount());
    }

    return lengthByteSize + length;
}
size_t BDP::readData(uint8_t lengthByteSize, const uint8_t* input, std::ostream& output, size_t* outputLength) {
    size_t length = 0u;

    bytesToLength(length, input, lengthByteSize);

    output.write((char*) (input + lengthByteSize), length);

    if(outputLength != nullptr)
        *outputLength = length;

    return lengthByteSize + length;
}

size_t BDP::getMaxLength(uint8_t lengthBitSize) {
    if(lengthBitSize == 8u)
        return (uint8_t) - 1;
    if(lengthBitSize == 16u)
        return (uint16_t) - 1;
    if(lengthBitSize == 32u)
        return (uint32_t) - 1;
    if(lengthBitSize == 64u)
        return (size_t) - 1;

    throw std::invalid_argument("lengthBitSize");
}

bool BDP::isLittleEndian() {
    union {
        uint32_t value;
        uint8_t bytes[4];
    } check = { 0x01020304 };
    return check.bytes[0] == 0x04;
}

void BDP::lengthToBytes(uint8_t* destination, size_t source, uint8_t count) {
    checkByteSize(count);

    if(isLittleEndian())
        valueToBytes(destination, source, count);
    else reversedValueToBytes(destination, source, count);
}

void BDP::bytesToLength(size_t& destination, const uint8_t* source, uint8_t count) {
    checkByteSize(count);

    if(isLittleEndian())
        bytesToValue(destination, source, count);
    else reversedBytesToValue(destination, source, count);
}

void BDP::reversedValueToBytes(uint8_t* destination, size_t source, uint8_t count) {
    uint8_t* src = reinterpret_cast<uint8_t*>(&source);
    destination += count - 1;

    while(count--)
        *destination-- = *src++;
}

void BDP::reversedBytesToValue(size_t& destination, const uint8_t* source, uint8_t count) {
    uint8_t* dst = reinterpret_cast<uint8_t*>(&destination);
    dst += count - 1;

    while(count--)
        *dst-- = *source++;
}

// The most common cases are 4 and 1, so order the if statements in their favor.
void BDP::valueToBytes(uint8_t* destination, size_t source, uint8_t count) {
    if(count == 4u)
        *reinterpret_cast<uint32_t*>(destination) = *reinterpret_cast<uint32_t*>(&source);
    else if(count == 1u)
        *destination = *reinterpret_cast<uint8_t*>(&source);
    else if(count == 8u)
        *reinterpret_cast<uint64_t*>(destination) = source;
    else *reinterpret_cast<uint16_t*>(destination) = *reinterpret_cast<uint16_t*>(&source);
}

// The most common cases are 4 and 1, so order the if statements in their favor.
void BDP::bytesToValue(size_t& destination, const uint8_t* source, uint8_t count) {
    if(count == 4u)
        destination = *reinterpret_cast<const uint32_t*>(source);
    else if(count == 1u)
        destination = *source;
    else if(count == 8u)
        destination = *reinterpret_cast<const uint64_t*>(source);
    else destination = *reinterpret_cast<const uint16_t*>(source);
}
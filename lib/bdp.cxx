/**
 * BDP (https://github.com/UnexomWid/BDP)
 *
 * This project is licensed under the MIT license.
 * Copyright (c) 2019 UnexomWid (https://uw.exom.dev)
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

/// The magic BDP value, which is located at the beginning of every BDP package.
const char* MAGIC_VALUE = "BDP";
/// The magic BDP value length.
const uint8_t MAGIC_VALUE_LENGTH = 3u;
/// The default size of the buffer used to copy data from one stream to another.
const uint64_t DEFAULT_BUFFER_SIZE = 16384u;

BDP::Header::Header(uint8_t nlbs, uint8_t vlbs) : NAME_MAX_LENGTH(getMaxLength(nlbs)),
                                                  VALUE_MAX_LENGTH(getMaxLength(vlbs)),
                                                  NAME_LENGTH_BIT_SIZE(nlbs),
                                                  VALUE_LENGTH_BIT_SIZE(vlbs),
                                                  NAME_LENGTH_BYTE_SIZE(nlbs / 8u),
                                                  VALUE_LENGTH_BYTE_SIZE(vlbs / 8u) { }

BDP::Header* BDP::writeHeader(std::ostream &output, uint8_t nameLengthBitSize, uint8_t valueLengthBitSize) {
    if (nameLengthBitSize != 8u && nameLengthBitSize != 16u && nameLengthBitSize != 32u && nameLengthBitSize != 64u)
        throw std::invalid_argument("nameLengthBitSize");
    if (valueLengthBitSize != 8u && valueLengthBitSize != 16u && valueLengthBitSize != 32u && valueLengthBitSize != 64u)
        throw std::invalid_argument("valueLengthBitSize");

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

    uint8_t lengthBitSizes = (((nameLengthBitSize) << 1u) | (valueLengthBitSize >> 3u));

    memcpy(output, MAGIC_VALUE, MAGIC_VALUE_LENGTH);
    memcpy(output + MAGIC_VALUE_LENGTH, &lengthBitSizes, 1u);

    return new BDP::Header(nameLengthBitSize, valueLengthBitSize);
}

uint64_t BDP::writeName(const BDP::Header* header, std::ostream &output, const uint8_t* name, uint64_t nameLength) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, nameLength);
}
uint64_t BDP::writeName(const BDP::Header* header, uint8_t* output, const uint8_t* name, uint64_t nameLength) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, nameLength);
}
uint64_t BDP::writeName(const BDP::Header* header, std::ostream &output, std::istream &name) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, DEFAULT_BUFFER_SIZE);
}
uint64_t BDP::writeName(const BDP::Header* header, std::ostream &output, std::istream &name, uint64_t bufferSize) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, bufferSize);
}
uint64_t BDP::writeName(const BDP::Header* header, uint8_t* output, std::istream &name) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, DEFAULT_BUFFER_SIZE);
}
uint64_t BDP::writeName(const BDP::Header* header, uint8_t* output, std::istream &name, uint64_t bufferSize) {
    return writeData(header->NAME_MAX_LENGTH, header->NAME_LENGTH_BYTE_SIZE, output, name, bufferSize);
}

uint64_t BDP::writeValue(const BDP::Header* header, std::ostream &output, const uint8_t* value, uint64_t valueLength) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, valueLength);
}
uint64_t BDP::writeValue(const BDP::Header* header, uint8_t* output, const uint8_t* value, uint64_t valueLength) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, valueLength);
}
uint64_t BDP::writeValue(const BDP::Header* header, std::ostream &output, std::istream &value) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, DEFAULT_BUFFER_SIZE);
}
uint64_t BDP::writeValue(const BDP::Header* header, std::ostream &output, std::istream &value, uint64_t bufferSize) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, bufferSize);
}
uint64_t BDP::writeValue(const BDP::Header* header, uint8_t* output, std::istream &value) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, DEFAULT_BUFFER_SIZE);
}
uint64_t BDP::writeValue(const BDP::Header* header, uint8_t* output, std::istream &value, uint64_t bufferSize) {
    return writeData(header->VALUE_MAX_LENGTH, header->VALUE_LENGTH_BYTE_SIZE, output, value, bufferSize);
}

uint64_t BDP::writePair(const BDP::Header* header, std::ostream &output, const uint8_t* name, uint64_t nameLength, const uint8_t* value, uint64_t valueLength) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output, value, valueLength);
}
uint64_t BDP::writePair(const BDP::Header* header, uint8_t* output, const uint8_t* name, uint64_t nameLength, const uint8_t* value, uint64_t valueLength) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output + header->NAME_LENGTH_BYTE_SIZE + nameLength, value, valueLength);
}
uint64_t BDP::writePair(const BDP::Header* header, std::ostream &output, const uint8_t* name, uint64_t nameLength, std::istream &value) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output, value);
}
uint64_t BDP::writePair(const BDP::Header* header, uint8_t* output, const uint8_t* name, uint64_t nameLength, std::istream &value) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output + header->NAME_LENGTH_BYTE_SIZE + nameLength, value);
}
uint64_t BDP::writePair(const BDP::Header* header, std::ostream &output, const uint8_t* name, uint64_t nameLength, std::istream &value, uint64_t bufferSize) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output, value, bufferSize);
}
uint64_t BDP::writePair(const BDP::Header* header, uint8_t* output, const uint8_t* name, uint64_t nameLength, std::istream &value, uint64_t bufferSize) {
    return writeName(header, output, name, nameLength) +
           writeValue(header, output + header->NAME_LENGTH_BYTE_SIZE + nameLength, value, bufferSize);
}
uint64_t BDP::writePair(const BDP::Header* header, std::ostream &output, std::istream &name, const uint8_t* value, uint64_t valueLength) {
    return writeName(header, output, name) +
           writeValue(header, output, value, valueLength);
}
uint64_t BDP::writePair(const BDP::Header* header, uint8_t* output, std::istream &name, const uint8_t* value, uint64_t valueLength) {
    uint64_t count = writeName(header, output, name);
    return count + writeValue(header, output + count, value, valueLength);
}
uint64_t BDP::writePair(const BDP::Header* header, std::ostream &output, std::istream &name, const uint8_t* value, uint64_t valueLength, uint64_t bufferSize) {
    return writeName(header, output, name, bufferSize) +
           writeValue(header, output, value, valueLength);
}
uint64_t BDP::writePair(const BDP::Header* header, uint8_t* output, std::istream &name, const uint8_t* value, uint64_t valueLength, uint64_t bufferSize) {
    uint64_t count = writeName(header, output, name, bufferSize);
    return count + writeValue(header, output + count, value, valueLength);
}
uint64_t BDP::writePair(const BDP::Header* header, std::ostream &output, std::istream &name, std::istream &value) {
    return writeName(header, output, name) +
           writeValue(header, output, value);
}
uint64_t BDP::writePair(const BDP::Header* header, uint8_t* output, std::istream &name, std::istream &value) {
    uint64_t count = writeName(header, output, name);
    return count + writeValue(header, output + count, value);
}
uint64_t BDP::writePair(const BDP::Header* header, std::ostream &output, std::istream &name, std::istream &value, uint64_t bufferSize) {
    return writeName(header, output, name, bufferSize) +
           writeValue(header, output, value, bufferSize);
}
uint64_t BDP::writePair(const BDP::Header* header, uint8_t* output, std::istream &name, std::istream &value, uint64_t bufferSize) {
    uint64_t count = writeName(header, output, name, bufferSize);
    return count + writeValue(header, output + count, value, bufferSize);
}

uint64_t BDP::writeData(uint64_t maxLength, uint8_t lengthByteSize, std::ostream &output, const uint8_t* data, uint64_t dataLength) {
    uint8_t* dataLengthBytes = (uint8_t*) malloc(lengthByteSize);
    uintToBytes(dataLength, lengthByteSize, dataLengthBytes);

    output.write((char*) (&dataLengthBytes[0]), lengthByteSize);
    output.write((char*) (&data[0]), dataLength);

    free(dataLengthBytes);

    return lengthByteSize + dataLength;
}
uint64_t BDP::writeData(uint64_t maxLength, uint8_t lengthByteSize, uint8_t* output, const uint8_t* data, uint64_t dataLength) {
    uint8_t* dataLengthBytes = (uint8_t*) malloc(lengthByteSize);
    uintToBytes(dataLength, lengthByteSize, dataLengthBytes);

    memcpy(output, dataLengthBytes, lengthByteSize);
    memcpy(output + lengthByteSize, data, dataLength);

    free(dataLengthBytes);

    return lengthByteSize + dataLength;
}
uint64_t BDP::writeData(uint64_t maxLength, uint8_t lengthByteSize, std::ostream &output, std::istream &data, uint64_t bufferSize) {
    uint64_t inputLength = 0u;

    uint8_t* inputLengthBytes = (uint8_t*) malloc(lengthByteSize);
    uintToBytes(inputLength, lengthByteSize, inputLengthBytes);

    std::streampos lastPos = output.tellp();

    // Write a placeholder value, as the actual length is unknown yet.
    output.write((char*) (&inputLengthBytes[0]), lengthByteSize);

    char* buffer = (char*) malloc(bufferSize);
    uint64_t diff;
    uint64_t nextLength;

    while((diff = maxLength - inputLength) != 0u && !data.eof()) {
        nextLength = diff < bufferSize ? diff : bufferSize;
        data.read(buffer, nextLength);
        output.write(buffer, data.gcount());
        inputLength += data.gcount();
    }

    free(buffer);

    std::streampos endPos = output.tellp();
    output.seekp(lastPos);

    // Write the actual length.
    uintToBytes(inputLength, lengthByteSize, inputLengthBytes);
    output.write((char*) (&inputLengthBytes[0]), lengthByteSize);

    output.seekp(endPos);

    free(inputLengthBytes);

    return lengthByteSize + inputLength;
}
uint64_t BDP::writeData(uint64_t maxLength, uint8_t lengthByteSize, uint8_t* output, std::istream &data, uint64_t bufferSize) {
    uint64_t inputLength = 0u;

    uint8_t* inputLengthBytes = (uint8_t*) malloc(lengthByteSize);
    uintToBytes(inputLength, lengthByteSize, inputLengthBytes);

    uint64_t index = 0;

    // Write a placeholder value, as the actual length is unknown yet.
    memcpy(output, inputLengthBytes, lengthByteSize);

    index += lengthByteSize;

    uint64_t diff;
    uint64_t nextLength;

    while((diff = maxLength - inputLength) != 0u && !data.eof()) {
        nextLength = diff < bufferSize ? diff : bufferSize;
        data.read((char*) (output + index), nextLength);

        index += data.gcount();
        inputLength += data.gcount();
    }

    // Write the actual length.
    uintToBytes(inputLength, lengthByteSize, inputLengthBytes);
    memcpy(output, inputLengthBytes, lengthByteSize);

    free(inputLengthBytes);

    return lengthByteSize + inputLength;
}

BDP::Header* BDP::readHeader(std::istream &input) {
    char* magic = (char*) malloc(MAGIC_VALUE_LENGTH + 1u);
    input.read(magic, MAGIC_VALUE_LENGTH);

    magic[MAGIC_VALUE_LENGTH] = '\0';

    if(strcmp(magic, MAGIC_VALUE) != 0)
        throw std::invalid_argument("input");

    uint8_t nameLengthBitSize = 64u;
    uint8_t valueLengthBitSize = 64u;

    uint8_t *lengthBitSizes = (uint8_t*) malloc(1u);
    input.read((char*) (&lengthBitSizes[0]), 1u);

    for(uint8_t i = 7u; i >= 4u; --i, nameLengthBitSize >>= 1)
        if(lengthBitSizes[0] & (1u << i))
            break;

    if(nameLengthBitSize == 0)
        throw std::invalid_argument("input: Invalid package header");

    for(uint8_t i = 3u; i >= 0u; --i, valueLengthBitSize >>= 1)
        if (lengthBitSizes[0] & (1u << i))
            break;

    if(valueLengthBitSize == 0)
        throw std::invalid_argument("input: Invalid package header");

    free(lengthBitSizes);
    free(magic);

    return new BDP::Header(nameLengthBitSize, valueLengthBitSize);
}
BDP::Header* BDP::readHeader(uint8_t* input) {
    uint64_t index = 0;

    char* magic = (char*) malloc(MAGIC_VALUE_LENGTH + 1u);
    memcpy(magic, input, MAGIC_VALUE_LENGTH);

    index += MAGIC_VALUE_LENGTH;
    magic[MAGIC_VALUE_LENGTH] = '\0';

    if(strcmp(magic, MAGIC_VALUE) != 0)
        throw std::invalid_argument("input");

    uint8_t nameLengthBitSize = 64u;
    uint8_t valueLengthBitSize = 64u;

    uint8_t *lengthBitSizes = (uint8_t*) malloc(1u);
    memcpy(lengthBitSizes, input + index, 1);
    ++index;

    for(uint8_t i = 7u; i >= 4u; --i, nameLengthBitSize >>= 1)
        if(lengthBitSizes[0] & (1u << i))
            break;

    if(nameLengthBitSize == 0)
        throw std::invalid_argument("input: Invalid package header");

    for(uint8_t i = 3u; i >= 0u; --i, valueLengthBitSize >>= 1)
        if (lengthBitSizes[0] & (1u << i))
            break;

    if(valueLengthBitSize == 0)
        throw std::invalid_argument("input: Invalid package header");

    free(lengthBitSizes);
    free(magic);

    return new BDP::Header(nameLengthBitSize, valueLengthBitSize);
}

uint64_t BDP::readName(const BDP::Header* header, std::istream &input, uint8_t* &name, uint64_t* nameLength) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, nameLength);
}
uint64_t BDP::readName(const BDP::Header* header, uint8_t* input, uint8_t* &name, uint64_t* nameLength) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, nameLength);
}
uint64_t BDP::readName(const BDP::Header* header, std::istream &input, std::ostream &name) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, DEFAULT_BUFFER_SIZE);
}
uint64_t BDP::readName(const BDP::Header* header, std::istream &input, std::ostream &name, uint64_t bufferSize) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, bufferSize);
}
uint64_t BDP::readName(const BDP::Header* header, uint8_t* input, std::ostream &name, uint64_t* nameLength) {
    return readData(header->NAME_LENGTH_BYTE_SIZE, input, name, nameLength);
}

uint64_t BDP::readValue(const BDP::Header* header, std::istream &input, uint8_t* &value, uint64_t* valueLength) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, valueLength);
}
uint64_t BDP::readValue(const BDP::Header* header, uint8_t* input, uint8_t* &value, uint64_t* valueLength) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, valueLength);
}
uint64_t BDP::readValue(const BDP::Header* header, std::istream &input, std::ostream &value) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, DEFAULT_BUFFER_SIZE);
}
uint64_t BDP::readValue(const BDP::Header* header, std::istream &input, std::ostream &value, uint64_t bufferSize) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, bufferSize);
}
uint64_t BDP::readValue(const BDP::Header* header, uint8_t* input, std::ostream &value, uint64_t* valueLength) {
    return readData(header->VALUE_LENGTH_BYTE_SIZE, input, value, valueLength);
}

uint64_t BDP::readPair(const BDP::Header* header, std::istream &input, uint8_t* &name, uint64_t* nameLength, uint8_t* &value, uint64_t* valueLength) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value, valueLength);
}
uint64_t BDP::readPair(const BDP::Header* header, uint8_t* input, uint8_t* &name, uint64_t* nameLength, uint8_t* &value, uint64_t* valueLength) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value, valueLength);
}
uint64_t BDP::readPair(const BDP::Header* header, std::istream &input, uint8_t* &name, uint64_t* nameLength, std::ostream &value) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value);
}
uint64_t BDP::readPair(const BDP::Header* header, uint8_t* input, uint8_t* &name, uint64_t* nameLength, std::ostream &value, uint64_t* valueLength) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value, valueLength);
}
uint64_t BDP::readPair(const BDP::Header* header, std::istream &input, uint8_t* &name, uint64_t* nameLength, std::ostream &value, uint64_t bufferSize) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value, bufferSize);
}
uint64_t BDP::readPair(const BDP::Header* header, std::istream &input, std::ostream &name, uint8_t* &value, uint64_t* valueLength) {
    return readName(header, input, name) +
           readValue(header, input, value, valueLength);
}
uint64_t BDP::readPair(const BDP::Header* header, uint8_t* input, std::ostream &name, uint64_t* nameLength, uint8_t* &value, uint64_t* valueLength) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value, valueLength);
}
uint64_t BDP::readPair(const BDP::Header* header, std::istream &input, std::ostream &name, uint8_t* &value, uint64_t* valueLength, uint64_t bufferSize) {
    return readName(header, input, name, bufferSize) +
           readValue(header, input, value, valueLength);
}
uint64_t BDP::readPair(const BDP::Header* header, std::istream &input, std::ostream &name, std::ostream &value) {
    return readName(header, input, name) +
           readValue(header, input, value);
}
uint64_t BDP::readPair(const BDP::Header* header, uint8_t* input, std::ostream &name, uint64_t* nameLength, std::ostream &value, uint64_t* valueLength) {
    return readName(header, input, name, nameLength) +
           readValue(header, input, value, valueLength);
}
uint64_t BDP::readPair(const BDP::Header* header, std::istream &input, std::ostream &name, std::ostream &value, uint64_t bufferSize) {
    return readName(header, input, name, bufferSize) +
           readValue(header, input, value, bufferSize);
}

uint64_t BDP::readData(uint8_t lengthByteSize, std::istream &input, uint8_t* &output, uint64_t* outputLength) {
    uint8_t* outputLengthBytes = (uint8_t*) malloc(lengthByteSize);
    input.read((char*) &(outputLengthBytes[0]), lengthByteSize);

    uint64_t length = 0u;
    bytesToUInt(outputLengthBytes, lengthByteSize, length);

    input.read((char*) (&output[0]), length);

    if(outputLength != nullptr)
        *outputLength = length;

    free(outputLengthBytes);

    return lengthByteSize + length;
}
uint64_t BDP::readData(uint8_t lengthByteSize, uint8_t* input, uint8_t* &output, uint64_t* outputLength) {
    uint8_t* outputLengthBytes = (uint8_t*) malloc(lengthByteSize);
    memcpy(outputLengthBytes, input, lengthByteSize);

    uint64_t length = 0u;
    bytesToUInt(outputLengthBytes, lengthByteSize, length);

    memcpy(output, input + lengthByteSize, length);

    if(outputLength != nullptr)
        *outputLength = length;

    free(outputLengthBytes);

    return lengthByteSize + length;
}
uint64_t BDP::readData(uint8_t lengthByteSize, std::istream &input, std::ostream &output, uint64_t bufferSize) {
    uint8_t* outputLengthBytes = (uint8_t*) malloc(lengthByteSize);
    input.read((char*) &(outputLengthBytes[0]), lengthByteSize);

    uint64_t length = 0u;
    bytesToUInt(outputLengthBytes, lengthByteSize, length);

    char* buffer = (char*) malloc(bufferSize);
    uint64_t nextLength;

    while(length > 0u && !input.eof()) {
        nextLength = length < bufferSize ? length : bufferSize;
        input.read(buffer, nextLength);
        output.write(buffer, input.gcount());
        length -= input.gcount();
    }

    free(buffer);
    free(outputLengthBytes);

    return lengthByteSize + length;
}
uint64_t BDP::readData(uint8_t lengthByteSize, uint8_t* input, std::ostream &output, uint64_t* outputLength) {
    uint8_t* outputLengthBytes = (uint8_t*) malloc(lengthByteSize);
    memcpy(outputLengthBytes, input, lengthByteSize);

    uint64_t length = 0u;
    bytesToUInt(outputLengthBytes, lengthByteSize, length);

    output.write((char*) (input + lengthByteSize), length);

    if(outputLength != nullptr)
        *outputLength = length;

    free(outputLengthBytes);

    return lengthByteSize + length;
}

uint64_t BDP::getMaxLength(uint8_t lengthBitSize) {
    if(lengthBitSize == 8u)
        return (uint8_t) - 1;
    if(lengthBitSize == 16u)
        return (uint16_t) - 1;
    if(lengthBitSize == 32u)
        return (uint32_t) - 1;
    if(lengthBitSize == 64u)
        return (uint64_t) - 1;

    throw std::invalid_argument("lengthBitSize");
}

void BDP::uintToBytes(uint64_t value, uint8_t byteCount, uint8_t *&destination) {
    for (uint8_t i = 0; i < byteCount; ++i)
        destination[i] = (uint8_t) (value >> (8u * (byteCount - i - 1u)));
}

void BDP::bytesToUInt(const uint8_t *bytes, uint8_t byteCount, uint64_t &destination) {
    destination = 0u;
    for (uint8_t i = 0; i < byteCount; ++i)
        destination |= (((uint64_t) bytes[i]) << (8u * (byteCount - i - 1u)));
}
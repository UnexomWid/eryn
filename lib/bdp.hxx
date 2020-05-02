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

#ifndef BDP_HXX_INCLUDED
#define BDP_HXX_INCLUDED

#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>

namespace BDP {
    struct Header {
        Header(uint8_t nlbs, uint8_t vlbs);

        const size_t NAME_MAX_LENGTH;
        const size_t VALUE_MAX_LENGTH;

        const uint8_t NAME_LENGTH_BIT_SIZE;
        const uint8_t VALUE_LENGTH_BIT_SIZE;

        const uint8_t NAME_LENGTH_BYTE_SIZE;
        const uint8_t VALUE_LENGTH_BYTE_SIZE;
    };

    Header* writeHeader(std::ostream& output, uint8_t nameLengthBitSize, uint8_t valueLengthBitSize);
    Header* writeHeader(uint8_t* output, uint8_t nameLengthBitSize, uint8_t valueLengthBitSize);

    size_t writeName(const Header* header, std::ostream& output, const uint8_t* name, size_t nameLength);
    size_t writeName(const Header* header, uint8_t* output, const uint8_t* name, size_t nameLength);
    size_t writeName(const Header* header, std::ostream& output, std::istream& name);
    size_t writeName(const Header* header, std::ostream& output, std::istream& name, size_t bufferSize);
    size_t writeName(const Header* header, uint8_t* output, std::istream& name);
    size_t writeName(const Header* header, uint8_t* output, std::istream& name, size_t bufferSize);

    size_t writeValue(const Header* header, std::ostream& output, const uint8_t* value, size_t valueLength);
    size_t writeValue(const Header* header, uint8_t* output, const uint8_t* value, size_t valueLength);
    size_t writeValue(const Header* header, std::ostream& output, std::istream& value);
    size_t writeValue(const Header* header, std::ostream& output, std::istream& value, size_t bufferSize);
    size_t writeValue(const Header* header, uint8_t* output, std::istream& value);
    size_t writeValue(const Header* header, uint8_t* output, std::istream& value, size_t bufferSize);

    size_t writePair(const Header* header, std::ostream& output, const uint8_t* name, size_t nameLength, const uint8_t* value, size_t valueLength);
    size_t writePair(const Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, const uint8_t* value, size_t valueLength);
    size_t writePair(const Header* header, std::ostream& output, const uint8_t* name, size_t nameLength, std::istream& value);
    size_t writePair(const Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, std::istream& value);
    size_t writePair(const Header* header, std::ostream& output, const uint8_t* name, size_t nameLength, std::istream& value, size_t bufferSize);
    size_t writePair(const Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, std::istream& value, size_t bufferSize);
    size_t writePair(const Header* header, std::ostream& output, std::istream& name, const uint8_t* value, size_t valueLength);
    size_t writePair(const Header* header, uint8_t* output, std::istream& name, const uint8_t* value, size_t valueLength);
    size_t writePair(const Header* header, std::ostream& output, std::istream& name, const uint8_t* value, size_t valueLength, size_t bufferSize);
    size_t writePair(const Header* header, uint8_t* output, std::istream& name, const uint8_t* value, size_t valueLength, size_t bufferSize);
    size_t writePair(const Header* header, std::ostream& output, std::istream& name, std::istream& value);
    size_t writePair(const Header* header, uint8_t* output, std::istream& name, std::istream& value);
    size_t writePair(const Header* header, std::ostream& output, std::istream& name, std::istream& value, size_t bufferSize);
    size_t writePair(const Header* header, uint8_t* output, std::istream& name, std::istream& value, size_t bufferSize);

    size_t writeData(size_t maxLength, uint8_t lengthByteSize, std::ostream& output, const uint8_t *data, size_t dataLength);
    size_t writeData(size_t maxLength, uint8_t lengthByteSize, uint8_t* output, const uint8_t *data, size_t dataLength);
    size_t writeData(size_t maxLength, uint8_t lengthByteSize, std::ostream& output, std::istream& data, size_t bufferSize);
    size_t writeData(size_t maxLength, uint8_t lengthByteSize, uint8_t* output, std::istream& data, size_t bufferSize);

    Header* readHeader(std::istream& input);
    Header* readHeader(const uint8_t* input);

    size_t readName(const Header* header, std::istream& input, uint8_t* name, size_t* nameLength);
    size_t readName(const Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength);
    size_t readName(const Header* header, std::istream& input, std::ostream& name);
    size_t readName(const Header* header, std::istream& input, std::ostream& name, size_t bufferSize);
    size_t readName(const Header* header, const uint8_t* input, std::ostream& name, size_t* nameLength);

    size_t readValue(const Header* header, std::istream& input, uint8_t* value, size_t* valueLength);
    size_t readValue(const Header* header, const uint8_t* input, uint8_t* value, size_t* valueLength);
    size_t readValue(const Header* header, std::istream& input, std::ostream& value);
    size_t readValue(const Header* header, std::istream& input, std::ostream& value, size_t bufferSize);
    size_t readValue(const Header* header, const uint8_t* input, std::ostream& value, size_t* valueLength);

    size_t readPair(const Header* header, std::istream& input, uint8_t* name, size_t* nameLength, uint8_t* value, size_t* valueLength);
    size_t readPair(const Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength, uint8_t* value, size_t* valueLength);
    size_t readPair(const Header* header, std::istream& input, uint8_t* name, size_t* nameLength, std::ostream& value);
    size_t readPair(const Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength, std::ostream& value, size_t* valueLength);
    size_t readPair(const Header* header, std::istream& input, uint8_t* name, size_t* nameLength, std::ostream& value, size_t bufferSize);
    size_t readPair(const Header* header, std::istream& input, std::ostream& name, uint8_t* value, size_t* valueLength);
    size_t readPair(const Header* header, const uint8_t* input, std::ostream& name, size_t* nameLength, uint8_t* value, size_t* valueLength);
    size_t readPair(const Header* header, std::istream& input, std::ostream& name, uint8_t* value, size_t* valueLength, size_t bufferSize);
    size_t readPair(const Header* header, std::istream& input, std::ostream& name, std::ostream& value);
    size_t readPair(const Header* header, const uint8_t* input, std::ostream& name, size_t* nameLength, std::ostream& value, size_t* valueLength);
    size_t readPair(const Header* header, std::istream& input, std::ostream& name, std::ostream& value, size_t bufferSize);

    size_t readData(uint8_t lengthByteSize, std::istream& input, uint8_t *&output, size_t* outputLength);
    size_t readData(uint8_t lengthByteSize, const uint8_t* input, uint8_t *&output, size_t* outputLength);
    size_t readData(uint8_t lengthByteSize, std::istream& input, std::ostream& output, size_t bufferSize);
    size_t readData(uint8_t lengthByteSize, const uint8_t* input, std::ostream& output,size_t* outputLength);

    size_t getMaxLength(uint8_t lengthBitSize);

    bool isLittleEndian();

    void lengthToBytes(uint8_t* destination, size_t source, uint8_t count);
    void bytesToLength(size_t& destination, const uint8_t* source, uint8_t count);

    void valueToBytes(uint8_t* destination, size_t source, uint8_t count);
    void bytesToValue(size_t& destination, const uint8_t* source, uint8_t count);

    void reversedValueToBytes(uint8_t* destination, size_t source, uint8_t count);
    void reversedBytesToValue(size_t& destination, const uint8_t* source, uint8_t count);
}

#endif
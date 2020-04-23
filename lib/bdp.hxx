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

/// Contains functions and structures used for working with BDP packages.
namespace BDP {

    /// The header of a BDP package, which contains package metadata.
    struct Header {
        /**
         * Initializes a new instance of the Header structure.
         *
         * @param[in] nlbs The bit size of the name length.
         * @param[in] vlbs The bit size of the value length.
         */
        Header(uint8_t nlbs, uint8_t vlbs);

        /// The maximum name length.
        const size_t NAME_MAX_LENGTH;
        /// The maximum value length.
        const size_t VALUE_MAX_LENGTH;

        /// The bit size of the name length.
        const uint8_t NAME_LENGTH_BIT_SIZE;
        /// The bit size of the value length.
        const uint8_t VALUE_LENGTH_BIT_SIZE;

        /// The byte size of the name length.
        const uint8_t NAME_LENGTH_BYTE_SIZE;
        /// The byte size of the name length.
        const uint8_t VALUE_LENGTH_BYTE_SIZE;
    };

    /**
     * Generates and writes a package header to a stream.
     *
     * @param[in, out] output The stream where to write the header.
     * @param[in] nameLengthBitSize The package name length bit size.
     * @param[in] valueLengthBitSize The package value length bit size.
     *
     * @return The generated package header.
     *
     * @note This function generates a package header from the passed arguments, and
     * writes it to a stream. After the header was written successfully, the
     * generated header is returned, as it is required for writing data to
     * the package.
     */
    Header* writeHeader(std::ostream &output, uint8_t nameLengthBitSize, uint8_t valueLengthBitSize);
    /**
     * Generates and writes a package header to a byte array.
     *
     * @param[in, out] output The byte array where to write the header.
     * @param[in] nameLengthBitSize The package name length bit size.
     * @param[in] valueLengthBitSize The package value length bit size.
     *
     * @return The generated package header.
     *
     * @note This function generates a package header from the passed arguments, and
     * writes it to a byte array. After the header was written successfully, the
     * generated header is returned, as it is required for writing data to
     * the package.
     */
    Header* writeHeader(uint8_t* output, uint8_t nameLengthBitSize, uint8_t valueLengthBitSize);

    /**
     * Writes a byte array representing a name to an output stream.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the name.
     * @param[in] name The byte array representing the name.
     * @param[in] nameLength The length of the byte array representing the name.
     *
     * @return How many bytes were written to the output, including the name length bytes.
     *
     * @note The output stream should be a BDP package. After writing the name,
     * a value should also be written to the stream, in order for the package
     * to be readable. Writing two consecutive names is not advised, as it
     * will, most likely, render the package partially unreadable.
     */
    size_t writeName(const Header* header, std::ostream &output, const uint8_t* name, size_t nameLength);
    /**
     * Writes a byte array representing a name to another byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the name.
     * @param[in] name The byte array representing the name.
     * @param[in] nameLength The length of the byte array representing the name.
     *
     * @return How many bytes were written to the output, including the name length bytes.
     *
     * @note The output stream should be a BDP package. After writing the name,
     * a value should also be written to the stream, in order for the package
     * to be readable. Writing two consecutive names is not advised, as it
     * will, most likely, render the package partially unreadable.
     */
    size_t writeName(const Header* header, uint8_t* output, const uint8_t* name, size_t nameLength);
    /**
     * Writes a stream representing a name to an output stream.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the name.
     * @param[in, out] name The stream representing the name.
     *
     * @return How many bytes were written to the output, including the name length bytes.
     *
     * @note This function will read from the name input stream until either EOF
     * or the maximum name length, defined in the package header, is reached. The
     * default buffer size will be used. The output stream should be a BDP package.
     * After writing the name a value should also be written to the stream, in order
     * for the package to be readable. Writing two consecutive names is not advised,
     * as it will, most likely, render the package partially unreadable.
     */
    size_t writeName(const Header* header, std::ostream &output, std::istream &name);
    /**
     * Writes a stream representing a name to an output stream, using a
     * specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the name.
     * @param[in, out] name The stream representing the name.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the name length bytes.
     *
     * @note This function will read from the name input stream until either EOF
     * or the maximum name length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     * After writing the name, a value should also be written to the stream, in order
     * for the package to be readable. Writing two consecutive names is not advised,
     * as it will, most likely, render the package partially unreadable.
     */
    size_t writeName(const Header* header, std::ostream &output, std::istream &name, size_t bufferSize);
    /**
     * Writes a stream representing a name to a byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the name.
     * @param[in, out] name The stream representing the name.
     *
     * @return How many bytes were written to the output, including the name length bytes.
     *
     * @note This function will read from the name input stream until either EOF
     * or the maximum name length, defined in the package header, is reached. The
     * default buffer size will be used. The output stream should be a BDP package.
     * After writing the name a value should also be written to the stream, in order
     * for the package to be readable. Writing two consecutive names is not advised,
     * as it will, most likely, render the package partially unreadable.
     */
    size_t writeName(const Header* header, uint8_t* output, std::istream &name);
    /**
     * Writes a stream representing a name to a byte array, using a
     * specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the name.
     * @param[in, out] name The stream representing the name.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the name length bytes.
     *
     * @note This function will read from the name input stream until either EOF
     * or the maximum name length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     * After writing the name, a value should also be written to the stream, in order
     * for the package to be readable. Writing two consecutive names is not advised,
     * as it will, most likely, render the package partially unreadable.
     */
    size_t writeName(const Header* header, uint8_t* output, std::istream &name, size_t bufferSize);

    /**
     * Writes a byte array representing a value to an output stream.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the value.
     * @param[in] value The byte array representing the value.
     * @param[in] valueLength The length of the byte array representing the value.
     *
     * @return How many bytes were written to the output, including the value length bytes.
     *
     * @note The output stream should be a BDP package. After writing the name,
     * a value should also be written to the stream, in order for the package
     * to be readable. Writing two consecutive values is not advised, as it
     * will, most likely, render the package partially unreadable.
     */
    size_t writeValue(const Header* header, std::ostream &output, const uint8_t* value, size_t valueLength);
    /**
     * Writes a byte array representing a value to another byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the value.
     * @param[in] value The byte array representing the value.
     * @param[in] valueLength The length of the byte array representing the value.
     *
     * @return How many bytes were written to the output, including the value length bytes.
     *
     * @note The output stream should be a BDP package. After writing the name,
     * a value should also be written to the stream, in order for the package
     * to be readable. Writing two consecutive values is not advised, as it
     * will, most likely, render the package partially unreadable.
     */
    size_t writeValue(const Header* header, uint8_t* output, const uint8_t* value, size_t valueLength);
    /**
     * Writes a stream representing a value to an output stream.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the value.
     * @param[in, out] value The stream representing the value.
     *
     * @return How many bytes were written to the output, including the value length bytes.
     *
     * @note This function will read from the value input stream until either EOF or the
     * maximum value length, defined in the package header, is reached. The default buffer
     * size will be used. The output stream should be a BDP package. Writing two consecutive
     * values is not advised, as it will, most likely, render the package partially unreadable.
     */
    size_t writeValue(const Header* header, std::ostream &output, std::istream &value);
    /**
     * Writes a stream representing a value to an output stream, using a specified
     * buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the value.
     * @param[in, out] value The stream representing the value.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the value length bytes.
     *
     * @note This function will read from the value input stream until either EOF
     * or the maximum value length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     * Writing two consecutive values is not advised, as it will, most likely, render
     * the package partially unreadable.
     */
    size_t writeValue(const Header* header, std::ostream &output, std::istream &value, size_t bufferSize);
    /**
     * Writes a stream representing a value to a byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the value.
     * @param[in, out] value The stream representing the value.
     *
     * @return How many bytes were written to the output, including the value length bytes.
     *
     * @note This function will read from the value input stream until either EOF or the
     * maximum value length, defined in the package header, is reached. The default buffer
     * size will be used. The output stream should be a BDP package. Writing two consecutive
     * values is not advised, as it will, most likely, render the package partially unreadable.
     */
    size_t writeValue(const Header* header, uint8_t* output, std::istream &value);
    /**
     * Writes a stream representing a value to a byte array, using a specified
     * buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the value.
     * @param[in, out] value The stream representing the value.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the value length bytes.
     *
     * @note This function will read from the value input stream until either EOF
     * or the maximum value length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     * Writing two consecutive values is not advised, as it will, most likely, render
     * the package partially unreadable.
     */
    size_t writeValue(const Header* header, uint8_t* output, std::istream &value, size_t bufferSize);

    /**
     * Writes a pair of a byte array representing a name, and a byte array representing
     * a value, to an output stream.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the pair.
     * @param[in] name The byte array representing the name.
     * @param[in] nameLength The length of the byte array representing the name.
     * @param[in] value The byte array representing the value.
     * @param[in] valueLength The length of the byte array representing the value.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, std::ostream &output, const uint8_t* name, size_t nameLength, const uint8_t* value, size_t valueLength);
    /**
     * Writes a pair of a byte array representing a name, and a byte array representing
     * a value, to another byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the pair.
     * @param[in] name The byte array representing the name.
     * @param[in] nameLength The length of the byte array representing the name.
     * @param[in] value The byte array representing the value.
     * @param[in] valueLength The length of the byte array representing the value.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, const uint8_t* value, size_t valueLength);
    /**
     * Writes a pair of a byte array representing a name, and a stream representing
     * a value, to an output stream.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the pair.
     * @param[in] name The byte array representing the name.
     * @param[in] nameLength The length of the byte array representing the name.
     * @param[in, out] value The stream representing the value.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the value input stream until either EOF
     * or the maximum value length, defined in the package header, is reached. The
     * default buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, std::ostream &output, const uint8_t* name, size_t nameLength, std::istream &value);
    /**
     * Writes a pair of a byte array representing a name, and a stream representing
     * a value, to another byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the pair.
     * @param[in] name The byte array representing the name.
     * @param[in] nameLength The length of the byte array representing the name.
     * @param[in, out] value The stream representing the value.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the value input stream until either EOF
     * or the maximum value length, defined in the package header, is reached. The
     * default buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, std::istream &value);
    /**
     * Writes a pair of a byte array representing a name, and a stream representing
     * a value, to an output stream, using a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the pair.
     * @param[in] name The byte array representing the name.
     * @param[in] nameLength The length of the byte array representing the name.
     * @param[in, out] value The stream representing the value.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the value input stream until either EOF
     * or the maximum value length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, std::ostream &output, const uint8_t* name, size_t nameLength, std::istream &value, size_t bufferSize);
    /**
     * Writes a pair of a byte array representing a name, and a stream representing
     * a value, to another byte array, using a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the pair.
     * @param[in] name The byte array representing the name.
     * @param[in] nameLength The length of the byte array representing the name.
     * @param[in, out] value The stream representing the value.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the value input stream until either EOF
     * or the maximum value length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, uint8_t* output, const uint8_t* name, size_t nameLength, std::istream &value, size_t bufferSize);
    /**
     * Writes a pair of a stream representing a name, and a byte array representing
     * a value, to an output stream.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the pair.
     * @param[in, out] name The stream representing the name.
     * @param[in] value The byte array representing the value.
     * @param[in] valueLength The length of the byte array representing the value.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the name input stream until either EOF
     * or the maximum name length, defined in the package header, is reached. The
     * default buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, std::ostream &output, std::istream &name, const uint8_t* value, size_t valueLength);
    /**
     * Writes a pair of a stream representing a name, and a byte array representing
     * a value, to another byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the pair.
     * @param[in, out] name The stream representing the name.
     * @param[in] value The byte array representing the value.
     * @param[in] valueLength The length of the byte array representing the value.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the name input stream until either EOF
     * or the maximum name length, defined in the package header, is reached. The
     * default buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, uint8_t* output, std::istream &name, const uint8_t* value, size_t valueLength);
    /**
     * Writes a pair of a stream representing a name, and a byte array representing
     * a value, to an output stream, using a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the pair.
     * @param[in, out] name The stream representing the name.
     * @param[in] value The byte array representing the value.
     * @param[in] valueLength The length of the byte array representing the value.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the name input stream until either EOF
     * or the maximum name length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, std::ostream &output, std::istream &name, const uint8_t* value, size_t valueLength, size_t bufferSize);
    /**
     * Writes a pair of a stream representing a name, and a byte array representing
     * a value, to another byte array, using a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the pair.
     * @param[in, out] name The stream representing the name.
     * @param[in] value The byte array representing the value.
     * @param[in] valueLength The length of the byte array representing the value.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the name input stream until either EOF
     * or the maximum name length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, uint8_t* output, std::istream &name, const uint8_t* value, size_t valueLength, size_t bufferSize);
    /**
     * Writes a pair of a stream representing a name, and a stream representing
     * a value, to an output stream.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the pair.
     * @param[in, out] name The stream representing the name.
     * @param[in, out] value The stream representing the value.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the name and value input streams until either EOF
     * or the maximum name/value length, defined in the package header, is reached. The
     * default buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, std::ostream &output, std::istream &name, std::istream &value);
    /**
     * Writes a pair of a stream representing a name, and a stream representing
     * a value, to a byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the pair.
     * @param[in, out] name The stream representing the name.
     * @param[in, out] value The stream representing the value.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the name and value input streams until either EOF
     * or the maximum name/value length, defined in the package header, is reached. The
     * default buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, uint8_t* output, std::istream &name, std::istream &value);
    /**
     * Writes a pair of a stream representing a name, and a stream representing
     * a value, to an output stream, using a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The stream where to write the pair.
     * @param[in, out] name The stream representing the name.
     * @param[in, out] value The stream representing the value.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the name and value input streams until either EOF
     * or the maximum name/value length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, std::ostream &output, std::istream &name, std::istream &value, size_t bufferSize);
    /**
     * Writes a pair of a stream representing a name, and a stream representing
     * a value, to a byte array, using a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] output The byte array where to write the pair.
     * @param[in, out] name The stream representing the name.
     * @param[in, out] value The stream representing the value.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the name and value length bytes.
     *
     * @note This function will read from the name and value input streams until either EOF
     * or the maximum name/value length, defined in the package header, is reached. The
     * specified buffer size will be used. The output stream should be a BDP package.
     */
    size_t writePair(const Header* header, uint8_t* output, std::istream &name, std::istream &value, size_t bufferSize);

    /**
     * Writes a byte array representing data to an output stream.
     *
     * @param[in] maxLength The maximum data length.
     * @param[in] lengthByteSize The data length byte size.
     * @param[in, out] output The stream where to write the data.
     * @param[in] data The byte array containing the data.
     * @param[in] dataLength The length of the byte array representing the length.
     *
     * @return How many bytes were written to the output, including the data length bytes.
     *
     * @see writeName
     * @see writeValue
     * @see writePair
     *
     * @note This function should not be used directly. Instead, use the writeName,
     * writeValue and writePair wrappers. However, if you intend to use it, make sure
     * to pass the correct maximum data length and length byte size (1, 2, 4 or 8)
     * corresponding to the length bit size of the type of data you are writing,
     * which is specified in the package header.
     */
    size_t writeData(size_t maxLength, uint8_t lengthByteSize, std::ostream &output, const uint8_t *data, size_t dataLength);
    /**
     * Writes a byte array representing data to another byte array.
     *
     * @param[in] maxLength The maximum data length.
     * @param[in] lengthByteSize The data length byte size.
     * @param[in, out] output The byte array to write the data.
     * @param[in] data The byte array containing the data.
     * @param[in] dataLength The length of the byte array representing the length.
     *
     * @return How many bytes were written to the output, including the data length bytes.
     *
     * @see writeName
     * @see writeValue
     * @see writePair
     *
     * @note This function should not be used directly. Instead, use the writeName,
     * writeValue and writePair wrappers. However, if you intend to use it, make sure
     * to pass the correct maximum data length and length byte size (1, 2, 4 or 8)
     * corresponding to the length bit size of the type of data you are writing,
     * which is specified in the package header.
     */
    size_t writeData(size_t maxLength, uint8_t lengthByteSize, uint8_t* output, const uint8_t *data, size_t dataLength);
    /**
     * Writes a stream representing data to an output stream, using a specified
     * buffer size.
     *
     * @param[in] maxLength The maximum data length.
     * @param[in] lengthByteSize The data length byte size.
     * @param[in, out] output The stream where to write the data.
     * @param[in, out] data The stream containing the data.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the data length bytes.
     *
     * @see writeName
     * @see writeValue
     * @see writePair
     *
     * @note This function should not be used directly. Instead, use the writeName,
     * writeValue and writePair wrappers. However, if you intend to use it, make sure
     * to pass the correct maximum data length and length byte size (1, 2, 4 or 8)
     * corresponding to the length bit size of the type of data you are writing,
     * which is specified in the package header.
     */
    size_t writeData(size_t maxLength, uint8_t lengthByteSize, std::ostream &output, std::istream &data, size_t bufferSize);
    /**
     * Writes a stream representing data to a byte array, using a specified
     * buffer size.
     *
     * @param[in] maxLength The maximum data length.
     * @param[in] lengthByteSize The data length byte size.
     * @param[in, out] output The byte array where to write the data.
     * @param[in, out] data The stream containing the data.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were written to the output, including the data length bytes.
     *
     * @see writeName
     * @see writeValue
     * @see writePair
     *
     * @note This function should not be used directly. Instead, use the writeName,
     * writeValue and writePair wrappers. However, if you intend to use it, make sure
     * to pass the correct maximum data length and length byte size (1, 2, 4 or 8)
     * corresponding to the length bit size of the type of data you are writing,
     * which is specified in the package header.
     */
    size_t writeData(size_t maxLength, uint8_t lengthByteSize, uint8_t* output, std::istream &data, size_t bufferSize);

    /**
     * Reads a package header from a stream.
     *
     * @param[in, out] input The stream from which to read the header.
     *
     * @return The package header.
     *
     * @note This function reads a package header from the stream, and
     * returns it, as it is required for reading data from the package.
     */
    Header* readHeader(std::istream &input);
    /**
     * Reads a package header from a byte array.
     *
     * @param[in, out] input The byte array from which to read the header.
     *
     * @return The package header.
     *
     * @note This function reads a package header from the byte array, and
     * returns it, as it is required for reading data from the package.
     */
    Header* readHeader(const uint8_t* input);

    /**
     * Reads a name from an input stream and writes it to a byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the name.
     * @param[out] name The byte array where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name length bytes.
     *
     * @note The input stream should be a BDP package. After reading the name,
     * a value should be read next from the stream, in order for the package
     * to be read as intended.
     */
    size_t readName(const Header* header, std::istream &input, uint8_t* name, size_t* nameLength);
    /**
     * Reads a name from a byte array and writes it to another byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] input The byte array from which to read the name.
     * @param[out] name The byte array where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name length bytes.
     *
     * @note The input stream should be a BDP package. After reading the name,
     * a value should be read next from the stream, in order for the package
     * to be read as intended.
     */
    size_t readName(const Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength);
    /**
     * Reads a name from an input stream and writes it to a stream.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the name.
     * @param[in, out] name The stream where to write the name to.
     *
     * @return How many bytes were read from the input, including the name length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP
     * package. After reading the name, a value should be read next from the stream,
     * in order for the package to be read as intended.
     */
    size_t readName(const Header* header, std::istream &input, std::ostream &name);
    /**
     * Reads a name from an input stream and writes it to a stream, using a specified
     * buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the name.
     * @param[in, out] name The stream where to write the name to.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were read from the input, including the name length bytes.
     *
     * @note The specified buffer size will be used. The input stream should be a BDP
     * package. After reading the name, a value should be read next from the stream,
     * in order for the package to be read as intended.
     */
    size_t readName(const Header* header, std::istream &input, std::ostream &name, size_t bufferSize);
    /**
     * Reads a name from a byte array and writes it to a stream.
     *
     * @param[in] header The package header.
     * @param[in, out] input The byte array from which to read the name.
     * @param[in, out] name The stream where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP
     * package. After reading the name, a value should be read next from the stream,
     * in order for the package to be read as intended.
     */
    size_t readName(const Header* header, const uint8_t* input, std::ostream &name, size_t* nameLength);

    /**
     * Reads a value from an input stream and writes it to a byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the value.
     * @param[out] value The byte array where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the value length bytes.
     *
     * @note The input stream should be a BDP package. After reading the value,
     * a name should be read next from the stream, in order for the package
     * to be read as intended.
     */
    size_t readValue(const Header* header, std::istream &input, uint8_t* value, size_t* valueLength);
    /**
     * Reads a value from a byte array and writes it to another byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] input The byte array from which to read the value.
     * @param[out] value The byte array where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the value length bytes.
     *
     * @note The input stream should be a BDP package. After reading the value,
     * a name should be read next from the stream, in order for the package
     * to be read as intended.
     */
    size_t readValue(const Header* header, const uint8_t* input, uint8_t* value, size_t* valueLength);
    /**
     * Reads a value from an input stream and writes it to a stream.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the value.
     * @param[in, out] value The stream where to write the value to.
     *
     * @return How many bytes were read from the input, including the value length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP
     * package. After reading the value, a name should be read next from the stream,
     * in order for the package to be read as intended.
     */
    size_t readValue(const Header* header, std::istream &input, std::ostream &value);
    /**
     * Reads a value from an input stream and writes it to a stream, using a specified
     * buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the value.
     * @param[in, out] value The stream where to write the value to.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were read from the input, including the value length bytes.
     *
     * @note The specified buffer size will be used. The input stream should be a BDP
     * package. After reading the value, a name should be read next from the stream,
     * in order for the package to be read as intended.
     */
    size_t readValue(const Header* header, std::istream &input, std::ostream &value, size_t bufferSize);
    /**
     * Reads a value from a byte array and writes it to a stream.
     *
     * @param[in] header The package header.
     * @param[in, out] input The byte array from which to read the value.
     * @param[in, out] value The stream where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the value length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP
     * package. After reading the value, a name should be read next from the stream,
     * in order for the package to be read as intended.
     */
    size_t readValue(const Header* header, const uint8_t* input, std::ostream &value, size_t* valueLength);

    /**
     * Reads a name and a value from an input stream and writes them to a pair of byte
     * arrays.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the pair.
     * @param[out] name The byte array where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     * @param[out] value The byte array where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, std::istream &input, uint8_t* name, size_t* nameLength, uint8_t* value, size_t* valueLength);
    /**
     * Reads a name and a value from a byte array, and writes them to a pair of byte
     * arrays.
     *
     * @param[in] header The package header.
     * @param[in, out] input The byte array from which to read the pair.
     * @param[out] name The byte array where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     * @param[out] value The byte array where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength, uint8_t* value, size_t* valueLength);
    /**
     * Reads a name and a value from an input stream and writes them to a pair of a byte array and
     * a stream.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the pair.
     * @param[out] name The byte array where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     * @param[in, out] value The stream where to write the value to.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, std::istream &input, uint8_t* name, size_t* nameLength, std::ostream &value);
    /**
     * Reads a name and a value from a byte array and writes them to a pair of a byte array and
     * a stream.
     *
     * @param[in] header The package header.
     * @param[in, out] input The byte array from which to read the pair.
     * @param[out] name The byte array where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     * @param[in, out] value The stream where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, const uint8_t* input, uint8_t* name, size_t* nameLength, std::ostream &value, size_t* valueLength);
    /**
     * Reads a name and a value from an input stream and writes them to a pair of a byte array and
     * a stream, using a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the pair.
     * @param[out] name The byte array where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     * @param[in, out] value The stream where to write the value to.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The specified buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, std::istream &input, uint8_t* name, size_t* nameLength, std::ostream &value, size_t bufferSize);
    /**
     * Reads a name and a value from an input stream and writes them to a pair of a stream and
     * a byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the pair.
     * @param[in, out] name The stream where to write the name to.
     * @param[out] value The byte array where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, std::istream &input, std::ostream &name, uint8_t* value, size_t* valueLength);
    /**
     * Reads a name and a value from a byte array and writes them to a pair of a stream and
     * a byte array.
     *
     * @param[in] header The package header.
     * @param[in, out] input The byte array from which to read the pair.
     * @param[in, out] name The stream where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     * @param[out] value The byte array where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, const uint8_t* input, std::ostream &name, size_t* nameLength, uint8_t* value, size_t* valueLength);
    /**
     * Reads a name and a value from an input stream and writes them to a pair of a stream and a
     * byte array, using a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the pair.
     * @param[in, out] name The stream where to write the name to.
     * @param[out] value The byte array where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The specified buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, std::istream &input, std::ostream &name, uint8_t* value, size_t* valueLength, size_t bufferSize);
    /**
     * Reads a name and a value from an input stream and writes them to a pair of streams.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the pair.
     * @param[in, out] name The stream where to write the name to.
     * @param[in, out] value The stream where to write the value to.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, std::istream &input, std::ostream &name, std::ostream &value);
    /**
     * Reads a name and a value from a byte array and writes them to a pair of streams.
     *
     * @param[in] header The package header.
     * @param[in, out] input The byte array from which to read the pair.
     * @param[in, out] name The stream where to write the name to.
     * @param[out] nameLength The memory location where to write the name length. Can be nullptr.
     * @param[in, out] value The stream where to write the value to.
     * @param[out] valueLength The memory location where to write the value length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The default buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, const uint8_t* input, std::ostream &name, size_t* nameLength, std::ostream &value, size_t* valueLength);
    /**
     * Reads a name and a value from an input stream and writes them to a pair of streams, using
     * a specified buffer size.
     *
     * @param[in] header The package header.
     * @param[in, out] input The stream from which to read the pair.
     * @param[in, out] name The stream where to write the name to.
     * @param[in, out] value The stream where to write the value to.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were read from the input, including the name and value length bytes.
     *
     * @note The specified buffer size will be used. The input stream should be a BDP package.
     */
    size_t readPair(const Header* header, std::istream &input, std::ostream &name, std::ostream &value, size_t bufferSize);

    /**
     * Reads data from an input stream and writes it to a byte array.
     *
     * @param[in] lengthByteSize The data length byte size.
     * @param[in, out] input The stream from which to read the data.
     * @param[in] output The byte array where to write the value to.
     * @param[in] outputLength The memory location where to write the data length. Can be nullptr
     *
     * @return How many bytes were read from the input, including the data length bytes.
     *
     * @see readName
     * @see readValue
     * @see readPair
     *
     * @note This function should not be used directly. Instead, use the readName,
     * readValue and readPair wrappers. However, if you intend to use it, make sure
     * to pass the correct length byte size (1, 2, 4 or 8) corresponding to the length
     * bit size of the type of data you are writing, which is specified in the package
     * header.
    */
    size_t readData(uint8_t lengthByteSize, std::istream &input, uint8_t *&output, size_t* outputLength);
    /**
     * Reads data from a byte array and writes it to a byte array.
     *
     * @param[in] lengthByteSize The data length byte size.
     * @param[in, out] input The byte array from which to read the data.
     * @param[in] output The byte array where to write the value to.
     * @param[in] outputLength The memory location where to write the data length. Can be nullptr
     *
     * @return How many bytes were read from the input, including the data length bytes.
     *
     * @see readName
     * @see readValue
     * @see readPair
     *
     * @note This function should not be used directly. Instead, use the readName,
     * readValue and readPair wrappers. However, if you intend to use it, make sure
     * to pass the correct length byte size (1, 2, 4 or 8) corresponding to the length
     * bit size of the type of data you are writing, which is specified in the package
     * header.
    */
    size_t readData(uint8_t lengthByteSize, const uint8_t* input, uint8_t *&output, size_t* outputLength);
    /**
     * Reads data from an input stream and writes it to a strean, using a specified
     * buffer size.
     *
     * @param[in] lengthByteSize The data length byte size.
     * @param[in, out] input The stream from which to read the data.
     * @param[in] data The stream where to write the value to.
     * @param[in] bufferSize The size of the buffer used to copy stream data.
     *
     * @return How many bytes were read from the input, including the data length bytes.
     *
     * @see readName
     * @see readValue
     * @see readPair
     *
     * @note This function should not be used directly. Instead, use the readName,
     * readValue and readPair wrappers. However, if you intend to use it, make sure
     * to pass the correct length byte size (1, 2, 4 or 8) corresponding to the length
     * bit size of the type of data you are writing, which is specified in the package
     * header.
    */
    size_t readData(uint8_t lengthByteSize, std::istream &input, std::ostream &output, size_t bufferSize);
    /**
     * Reads data from a byte array and writes it to a stream, using a specified
     * buffer size.
     *
     * @param[in] lengthByteSize The data length byte size.
     * @param[in, out] input The byte array from which to read the data.
     * @param[in] data The stream where to write the value to.
     * @param[in] outputLength The memory location where to write the data length. Can be nullptr.
     *
     * @return How many bytes were read from the input, including the data length bytes.
     *
     * @see readName
     * @see readValue
     * @see readPair
     *
     * @note This function should not be used directly. Instead, use the readName,
     * readValue and readPair wrappers. However, if you intend to use it, make sure
     * to pass the correct length byte size (1, 2, 4 or 8) corresponding to the length
     * bit size of the type of data you are writing, which is specified in the package
     * header.
    */
    size_t readData(uint8_t lengthByteSize, const uint8_t* input, std::ostream &output,size_t* outputLength);

    /**
     * Returns the maximum value that can be stored with a specified amount of bits.
     *
     * @param[in] lengthBitSize The bit size to get the maximum value of.
     *
     * @return The maximum value that can be stored with the specified amount of bits.
     *
     * @note The valid bit sizes are 8, 16, 32 and 64.
     */
    size_t getMaxLength(uint8_t lengthBitSize);

    /**
     * Checks whether or not the endianness of the platform is little-endian.
     *
     * @return True if the architecture uses little endian. False otherwise.
     */
    bool isLittleEndian();

    /**
     * Converts an unsigned integer to a byte array.
     *
     * @param[out] destination The byte array where to store the converted value.
     * @param[in] source The value to convert.
     * @param[in] count The number of bytes to convert the value to.
     */
    void lengthToBytes(uint8_t* destination, size_t source, uint8_t count);

    /**
     * Converts a byte array to an unsigned integer.
     *
     * @param[out] destination The variable where to store the converted byte array.
     * @param[in] source The byte array to convert.
     * @param[in] count The number of bytes to convert to an unsigned integer.
     */
    void bytesToLength(size_t &destination, const uint8_t* source, uint8_t count);

    /**
     * Directly converts an unsigned integer to a byte array.
     *
     * @param[out] destination The byte array where to store the converted value.
     * @param[in] source The value to convert.
     * @param[in] count The number of bytes to convert the value to.
     */
    void directLengthToBytes(uint8_t* destination, size_t source, uint8_t count);

    /**
     * Directly converts a byte array to an unsigned integer.
     *
     * @param[out] destination The variable where to store the converted byte array.
     * @param[in] source The byte array to convert.
     * @param[in] count The number of bytes to convert to an unsigned integer.
     */
    void directBytesToLength(size_t &destination, const uint8_t* source, uint8_t count);
}

#endif
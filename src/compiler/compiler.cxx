#pragma warning(disable : 4996)

#include "compiler.hxx"
#include "../../lib/bdp.hxx"
#include "../../lib/mem_find.h"
#include "../../lib/mem_index.h"
#include "../def/osh.dxx"
#include "../def/logging.dxx"
#include "../global/cache.hxx"
#include "../global/global.hxx"
#include "../global/options.hxx"
#include "../except/compilation.hxx"

#include <stack>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

using Global::Cache;
using Global::Options;

void compileFile(const char* wd, const char* path, const char* outputPath) {
    LOG_INFO("===> Compiling file '%s'\n", path);

    FILE* input = fopen(path, "rb");

    if(input == NULL)
        throw CompilationException("Compiler error", "cannot open input file");

    fseek(input, 0, SEEK_END);
    long fileLength = ftell(input);
    fseek(input, 0, SEEK_SET);

    LOG_DEBUG("File size is %ld bytes\n\n", fileLength);

    size_t inputSize = (size_t) fileLength;
    std::unique_ptr<uint8_t, decltype(qfree)*> inputBuffer(qmalloc(inputSize), qfree);

    fread(inputBuffer.get(), 1, inputSize, input);
    fclose(input);

    BinaryData compiled = compileBytes(inputBuffer.get(), inputSize, wd);

    LOG_DEBUG("Wrote %zd bytes to output\n", compiled.size)

    FILE* dest = fopen(outputPath, "wb");
    fwrite(compiled.data, 1, compiled.size, dest);
    fclose(dest);

    qfree((uint8_t*) compiled.data);
}

BinaryData compileBytes(uint8_t* input, size_t inputSize, const char* wd) {
    size_t   outputSize = 0;
    size_t   outputCapacity = inputSize;
    std::unique_ptr<uint8_t, decltype(qfree)*> output(qalloc(outputCapacity), qfree);

    uint8_t* start = input;
    uint8_t* end = input + mem_find(input, inputSize, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());
    size_t   length = end - start;

    uint8_t* limit = input + inputSize;

    size_t   remainingLength;
    size_t   index;
    size_t   templateStartIndex;
    size_t   templateEndIndex;

    enum class TemplateType {
        CONDITIONAL,
        LOOP,
        COMPONENT
    };

    struct TemplateStackInfo {
        TemplateType type;      // The template type.
        size_t outputEndIndex;  // At which index the template ends in the output (for writing the index of the template end).
        size_t inputStartIndex; // At which index the template starts in the input (provides more information when an exception occurs).

        TemplateStackInfo(TemplateType typ, size_t outEnd, size_t inStart) : type(typ), outputEndIndex(outEnd), inputStartIndex(inStart) { };
    };

    std::stack<TemplateStackInfo> templateStack;

    while(end < limit) {
        if(start != end) {
            LOG_DEBUG("--> Found plaintext at %zu\n", start - input);

            while(outputSize + 1 + OSH_PLAINTEXT_MARKER_LENGTH + 4 + length > outputCapacity) {
                uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                output.release();
                output.reset(newOutput);
            }

            LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - input, end - input);
            outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_MARKER_LENGTH, start, length);
            LOG_DEBUG("done\n\n");
        }
        templateStartIndex = end - input;

        LOG_DEBUG("--> Found template start at %zu\n", templateStartIndex);

        end += Options::getTemplateStartLength();

        while(*end == ' ' || *end == '\t')
            ++end;

        if(membcmp(end, Options::getTemplateConditionalStart(), Options::getTemplateConditionalStartLength())) {
            LOG_DEBUG("Detected template conditional start\n");

            end += Options::getTemplateConditionalStartLength();

            while(*end == ' ' || *end == '\t')
                ++end;

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);

                throw CompilationException("Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            if(index == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);           

                CompilationException exception("Unexpected template end", "did you forget to write the condition?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - input);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;
            ++end;

            if(start != end) { // Template conditional start.
                length = end - start;

                while(outputSize + 1 + OSH_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH + 4 + length + OSH_FORMAT > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template conditional start as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_CONDITIONAL_START_MARKER, OSH_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH, start, length);
                memset(output.get() + outputSize, 0, OSH_FORMAT);
                
                templateStack.push(TemplateStackInfo(TemplateType::CONDITIONAL, outputSize, templateStartIndex));
                outputSize += OSH_FORMAT;
                LOG_DEBUG("done\n\n");
            }

            end = start;
        } else if(membcmp(end, Options::getTemplateConditionalEnd(), Options::getTemplateConditionalEndLength())) {
            LOG_DEBUG("Detected template conditional end\n");

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);   

                throw CompilationException("Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - input);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;

            if(start == end) { // Template conditional end.
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);

                    throw CompilationException("Unexpected conditional end", "there is no conditional to close; delete this", ln, col, chunk.get(), chunkIndex, chunkSize);
                } else if(templateStack.top().type != TemplateType::CONDITIONAL) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, templateStack.top().inputStartIndex, &ln, &col);

                    std::string msgBuffer;
                    msgBuffer.reserve(64);

                    msgBuffer += "close the ";          

                    switch(templateStack.top().type) {
                        case TemplateType::LOOP:
                            msgBuffer += "loop";
                            break;
                        case TemplateType::COMPONENT:
                            msgBuffer += "component";
                            break;
                    }

                    msgBuffer += " template at ";
                    msgBuffer += std::to_string(ln);
                    msgBuffer += ":";
                    msgBuffer += std::to_string(col);
                    msgBuffer += " first";

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);

                    throw CompilationException("Unexpected conditional end", msgBuffer.c_str(), ln, col, chunk.get(), chunkIndex, chunkSize);
                }

                ++end;
                length = 0;

                while(outputSize + 1 + OSH_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH + 4 + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template conditional end as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_CONDITIONAL_END_MARKER, OSH_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH, start, length);

                if(BDP::isLittleEndian())
                    BDP::directLengthToBytes(output.get() + templateStack.top().outputEndIndex, outputSize, OSH_FORMAT);
                else BDP::lengthToBytes(output.get() + templateStack.top().outputEndIndex, outputSize, OSH_FORMAT);
                
                templateStack.pop();
                LOG_DEBUG("done\n\n");

                end = start;
            } else {
                LOG_DEBUG("Re-detected as ordinary template\n");

                ++end;
                length = end - start;

                while(outputSize + 1 + OSH_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n\n");

                end = start;
            }
        } else if(membcmp(end, Options::getTemplateLoopStart(), Options::getTemplateLoopStartLength())) {
            LOG_DEBUG("Detected template loop start\n");

            end += Options::getTemplateLoopStartLength();

            while(*end == ' ' || *end == '\t')
                ++end;

            start = end;

            uint8_t* leftStart = start;
            uint8_t* leftEnd = start;
            size_t   sepIndex;

            remainingLength = inputSize - (end - input);
            sepIndex = mem_find(end, remainingLength, Options::getTemplateLoopSeparator(), Options::getTemplateLoopSeparatorLength(), Options::getTemplateLoopSeparatorLookup());
            index    = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(sepIndex > index) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);           

                throw CompilationException("Unexpected end of template", "did you forget to write the loop separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(sepIndex == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + sepIndex) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);         

                throw CompilationException("Unexpected EOF", "did you forget to write the loop separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(sepIndex == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - input;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);           

                throw CompilationException("Unexpected separator", "did you forget to provide the left argument before the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);           

                throw CompilationException("Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template loop separator at %zu\n", end + sepIndex - input);
            LOG_DEBUG("Found template end at %zu\n", end + index - input);

            leftEnd = end + sepIndex - 1;
            start = end + sepIndex + Options::getTemplateLoopSeparatorLength();

            while(*leftEnd == ' ' || *leftEnd == '\t')
                --leftEnd;
            ++leftEnd;

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;
            ++end;

            if(end == start) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (leftStart + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);          

                throw CompilationException("Unexpected end of template", "did you forget to provide the right argument after the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            while(*start == ' ' || *start == '\t')
                ++start;

            length = end - start;
            size_t leftLength = leftEnd - leftStart;

            while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_LOOP_START_MARKER_LENGTH > outputCapacity) {
                uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                output.release();
                output.reset(newOutput);
            }

            LOG_DEBUG("Writing template loop start as BDP832 pair %zu -> %zu...", leftStart - input, end - input);
            outputSize += BDP::writeName(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_LOOP_START_MARKER, OSH_TEMPLATE_LOOP_START_MARKER_LENGTH);

            size_t tempBufferSize = Global::BDP832->VALUE_LENGTH_BYTE_SIZE * 2 + leftLength + length;
            uint8_t* tempBuffer = (uint8_t*) malloc(tempBufferSize);

            BDP::writeValue(Global::BDP832, tempBuffer, leftStart, leftLength);
            BDP::writeValue(Global::BDP832, tempBuffer + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength, start, length);

            while(outputSize + tempBufferSize + OSH_FORMAT > outputCapacity) {
                uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                output.release();
                output.reset(newOutput);
            }

            outputSize += BDP::writeValue(Global::BDP832, output.get() + outputSize, tempBuffer, tempBufferSize);
            memset(output.get() + outputSize, 0, OSH_FORMAT);
                
            templateStack.push(TemplateStackInfo(TemplateType::LOOP, outputSize, templateStartIndex));
            outputSize += OSH_FORMAT;
            free(tempBuffer);

            LOG_DEBUG("done\n\n");

            end = leftStart;
        } else if(membcmp(end, Options::getTemplateLoopEnd(), Options::getTemplateLoopEndLength())) {
            LOG_DEBUG("Detected template loop end\n");

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);          

                throw CompilationException("Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - input);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;

            if(start == end) { // Template loop end.
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);          

                    throw CompilationException("Unexpected loop end", "there is no loop to close; delete this", ln, col, chunk.get(), chunkIndex, chunkSize);
                } else if(templateStack.top().type != TemplateType::LOOP) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, templateStack.top().inputStartIndex, &ln, &col);

                    std::string msgBuffer;
                    msgBuffer.reserve(64);

                    msgBuffer += "close the";          

                    switch(templateStack.top().type) {
                        case TemplateType::CONDITIONAL:
                            msgBuffer += " conditional ";
                            break;
                        case TemplateType::COMPONENT:
                            msgBuffer += " component ";
                            break;
                    }

                    msgBuffer += "template at ";
                    msgBuffer += std::to_string(ln);
                    msgBuffer += ":";
                    msgBuffer += std::to_string(col);
                    msgBuffer += " first";

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);

                    throw CompilationException("Unexpected loop end", msgBuffer.c_str(), ln, col, chunk.get(), chunkIndex, chunkSize);
                }

                ++end;
                length = 0;

                if(outputSize + 1 + OSH_TEMPLATE_LOOP_END_MARKER_LENGTH + 4 + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template loop end as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_LOOP_END_MARKER, OSH_TEMPLATE_LOOP_END_MARKER_LENGTH, start, length);

                if(BDP::isLittleEndian())
                    BDP::directLengthToBytes(output.get() + templateStack.top().outputEndIndex, outputSize, OSH_FORMAT);
                else BDP::lengthToBytes(output.get() + templateStack.top().outputEndIndex, outputSize, OSH_FORMAT);
                
                templateStack.pop();
                LOG_DEBUG("done\n\n");

                end = start;
            } else {
                LOG_DEBUG("Re-detected as ordinary template\n");

                ++end;
                length = end - start;

                if(outputSize + 1 + OSH_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n\n");

                end = start;
            }
        } else if(membcmp(end, Options::getTemplateComponent(), Options::getTemplateComponentLength())) {
            LOG_DEBUG("Detected template component\n");

            end += Options::getTemplateComponentLength();

            while((*end == ' ' || *end == '\t') && end < input + inputSize)
                ++end;

            start = end;

            uint8_t* leftStart = start;
            uint8_t* leftEnd = start;
            size_t   sepIndex;

            remainingLength = inputSize - (end - input);
            sepIndex = mem_find(end, remainingLength, Options::getTemplateComponentSeparator(), Options::getTemplateComponentSeparatorLength(), Options::getTemplateComponentSeparatorLookup());
            index    = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(sepIndex == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - input;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);            

                throw CompilationException("Unexpected separator", "did you forget to provide the component name before the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);          

                throw CompilationException("Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } 
            
            if(index == 0) {
                LOG_DEBUG("Detected empty template component\n\n");
            } else {
                LOG_DEBUG("Found template component at %zu\n", end + sepIndex - input);
                LOG_DEBUG("Found template end at %zu\n", end + index - input);

                templateEndIndex = end + index - input;

                if(sepIndex < index) {
                    leftEnd = end + sepIndex - 1;

                    while(*leftEnd == ' ' || *leftEnd == '\t')
                        --leftEnd;
                    ++leftEnd;

                    start = end + sepIndex + Options::getTemplateComponentSeparatorLength();
                    end = end + index - 1;
                    while(*end == ' ' || *end == '\t')
                        --end;
                    ++end;

                    if(end == start) {
                        size_t ln;
                        size_t col;
                        size_t chunkIndex;
                        size_t chunkSize;
                        size_t errorIndex = (leftStart + index) - input - 1;

                        mem_lncol(input, errorIndex, &ln, &col);
                        std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                            mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);         

                        throw CompilationException("Unexpected end of template", "did you forget to provide the component context after the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
                    }

                    while(*start == ' ' || *start == '\t')
                        ++start;
                    length = end - start;
                } else {
                    leftEnd = end + index - 1;

                    while(*leftEnd == ' ' || *leftEnd == '\t')
                        --leftEnd;
                    ++leftEnd;

                    start = leftStart;
                    end = leftEnd;
                    length = 0;
                }

                size_t leftLength = leftEnd - leftStart;

                bool isSelf = false;
                uint8_t* selfStart;

                if(end - start + 1 >= Options::getTemplateComponentSelfLength()) {
                    selfStart = end - Options::getTemplateComponentSelfLength();
                    if(0 == mem_find(selfStart, inputSize - (selfStart - input), Options::getTemplateComponentSelf(), Options::getTemplateComponentSelfLength(), Options::getTemplateComponentSelfLookup())) {
                        LOG_DEBUG("Detected self-closing template component\n");

                        isSelf = true;
                        end = selfStart - 1;

                        while((*end == ' ' || *end == '\t') && end > start)
                            --end;
                        ++end;

                        if(length != 0)
                            length = end - start;
                        else {
                            leftEnd = end;
                            leftLength = leftEnd - leftStart;
                        }
                    }
                }

                if(end <= start) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = (selfStart) - input;

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);

                    throw CompilationException("Unexpected end of template", "did you forget to provide the component context after the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
                }

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_COMPONENT_MARKER_LENGTH > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template component as BDP832 pair %zu -> %zu...", leftStart - input, end - input);
                outputSize += BDP::writeName(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_COMPONENT_MARKER, OSH_TEMPLATE_COMPONENT_MARKER_LENGTH);

                size_t componentPathLength;
                std::unique_ptr<uint8_t, decltype(qfree)*> componentPath(
                    componentPathToAbsolute(wd, reinterpret_cast<const char*>(leftStart), leftLength, componentPathLength), qfree);

                size_t tempBufferSize = Global::BDP832->VALUE_LENGTH_BYTE_SIZE * 2 + componentPathLength + length;
                std::unique_ptr<uint8_t, decltype(qfree)*> tempBuffer(qmalloc(tempBufferSize), qfree);

                printf("\n%zu %s\n", componentPathLength, componentPath.get());

                BDP::writeValue(Global::BDP832, tempBuffer.get(), componentPath.get(), componentPathLength);
                BDP::writeValue(Global::BDP832, tempBuffer.get() + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + componentPathLength, start, length);

                while(outputSize + tempBufferSize + OSH_FORMAT > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                outputSize += BDP::writeValue(Global::BDP832, output.get() + outputSize, tempBuffer.get(), tempBufferSize);
                memset(output.get() + outputSize, 0, OSH_FORMAT);
                
                if(!isSelf)
                    templateStack.push(TemplateStackInfo(TemplateType::COMPONENT,outputSize, templateStartIndex));
                outputSize += OSH_FORMAT;

                LOG_DEBUG("done\n\n");

                end = leftStart;
            }
        } else if(membcmp(end, Options::getTemplateComponentEnd(), Options::getTemplateComponentEndLength())) {
            LOG_DEBUG("Detected template component end\n");

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);          

                throw CompilationException("Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - input);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;

            if(start == end) { // Template component end.
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);

                    throw CompilationException("Unexpected component end", "there is no component to close; delete this", ln, col, chunk.get(), chunkIndex, chunkSize);
                } else if(templateStack.top().type != TemplateType::COMPONENT) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, templateStack.top().inputStartIndex, &ln, &col);

                    std::string msgBuffer;
                    msgBuffer.reserve(64);

                    msgBuffer += "close the";          

                    switch(templateStack.top().type) {
                        case TemplateType::CONDITIONAL:
                            msgBuffer += " conditional ";
                            break;
                        case TemplateType::LOOP:
                            msgBuffer += " loop ";
                            break;
                    }

                    msgBuffer += "template at ";
                    msgBuffer += std::to_string(ln);
                    msgBuffer += ":";
                    msgBuffer += std::to_string(col);
                    msgBuffer += " first";

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);

                    throw CompilationException("Unexpected component end", msgBuffer.c_str(), ln, col, chunk.get(), chunkIndex, chunkSize);
                }

                ++end;
                length = 0;

                while(outputSize + 1 + OSH_TEMPLATE_COMPONENT_END_MARKER_LENGTH + 4 + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template component end as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_COMPONENT_END_MARKER, OSH_TEMPLATE_COMPONENT_END_MARKER_LENGTH, start, length);
                
                if(BDP::isLittleEndian())
                    BDP::directLengthToBytes(output.get() + templateStack.top().outputEndIndex, outputSize, OSH_FORMAT);
                else BDP::lengthToBytes(output.get() + templateStack.top().outputEndIndex, outputSize, OSH_FORMAT);
                
                templateStack.pop();
                LOG_DEBUG("done\n\n");

                end = start;
            } else {
                LOG_DEBUG("Re-detected as ordinary template\n");

                ++end;
                length = end - start;

                while(outputSize + 1 + OSH_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n\n");

                end = start;
            }
        } else { // Template.
            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = (end + index) - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);    

                throw CompilationException("Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(index != 0) {
                LOG_DEBUG("Found template end at %zu\n", end + index - input);

                end = end + index - 1;
                while(*end == ' ' || *end == '\t')
                    --end;
                ++end;

                length = end - start;

                if(outputSize + 1 + OSH_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n\n");

                end = start;
            } else {
                LOG_DEBUG("Detected empty template\n\n");
            }
        }

        start = input + templateEndIndex + Options::getTemplateEndLength();

        if(start >= limit)
            break;

        remainingLength = inputSize - (start - input);
        end = start + mem_find(start, remainingLength, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());
        length = end - start;
    }

    if(!templateStack.empty()) {
        size_t ln;
        size_t col;
        size_t chunkIndex;
        size_t chunkSize;
        size_t errorIndex = templateStack.top().inputStartIndex;

        mem_lncol(input, errorIndex, &ln, &col);
        std::unique_ptr<uint8_t, decltype(qfree)*> chunk(
            mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), qfree);

        std::string msgBuffer;
        msgBuffer.reserve(64);

        msgBuffer += "Expected end for";          

        switch(templateStack.top().type) {
            case TemplateType::CONDITIONAL:
                msgBuffer += " conditional ";
                break;
            case TemplateType::LOOP:
                msgBuffer += " loop ";
                break;
            case TemplateType::COMPONENT:
                msgBuffer += " component ";
                break;
        }

        msgBuffer += "template";      

        throw CompilationException(msgBuffer.c_str(), "did you forget to close this?", ln, col, chunk.get(), chunkIndex, chunkSize);
    }

    if(end > start) {
        LOG_DEBUG("--> Found plaintext at %zu\n", start - input);

        if(outputSize + 1 + OSH_PLAINTEXT_MARKER_LENGTH + 4 + length > outputCapacity) {
            uint8_t* newOutput = qexpand(output.get(), outputCapacity);
            output.release();
            output.reset(newOutput);
        }

        LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - input, end - input);
        outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_MARKER_LENGTH, start, length);
        LOG_DEBUG("done\n\n")
    }

    // Bring the capacity to the actual size.
    if(outputSize != outputCapacity) {
        uint8_t* newOutput = qrealloc(output.get(), outputSize);
        output.release();
        output.reset(newOutput);
    }

    uint8_t* compiled = output.get();
    output.release();

    return BinaryData(compiled, outputSize);
}

uint8_t* componentPathToAbsolute(const char* wd, const char* componentPath, size_t componentPathLength, size_t &absoluteLength) {
    if(*componentPath == '/' || *componentPath == '\\') {
        uint8_t* absolute = qmalloc(componentPathLength);
        memcpy(absolute, componentPath, componentPathLength);

        absoluteLength = componentPathLength;
        return absolute;
    }

    std::string pathBuilder(wd);
    pathBuilder.reserve(pathBuilder.size() + 256);

    if(pathBuilder[pathBuilder.size() - 1] != '/' && pathBuilder[pathBuilder.size() - 1] != '\\')
        pathBuilder += '/';
    pathBuilder.append(componentPath, componentPathLength);

    const char* absolute = strdup(pathBuilder.c_str());

    if(!absolute)
        throw MemoryException("Cannot allocate memory for component path", pathBuilder.size());

    absoluteLength = pathBuilder.size();
    return (uint8_t*) absolute;
}
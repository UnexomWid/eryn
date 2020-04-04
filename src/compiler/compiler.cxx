#pragma warning(disable : 4996)

#include "compiler.hxx"
#include "../def/osh.dxx"
#include "../../lib/bdp.hxx"
#include "../../lib/mem_find.h"
#include "../../lib/mem_index.h"
#include "../global/cache.hxx"
#include "../global/global.hxx"
#include "../global/options.hxx"
#include "../except/compilation.hxx"

#include <stack>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

#define REALLOC_STEP_SIZE 4096

using Global::Cache;
using Global::Options;

void compileFile(const char* path, const char* outputPath) {
    LOG_INFO("===> Compiling file '%s'\n", path);

    FILE* input = fopen(path, "rb");

    if(input == NULL)
        throw std::runtime_error("Cannot open file");

    fseek(input, 0, SEEK_END);
    long inputSize = ftell(input);
    fseek(input, 0, SEEK_SET);

    LOG_DEBUG("File size is %ld bytes\n\n", inputSize);

    uint8_t* inputBuffer = (uint8_t*) malloc(inputSize);

    fread(inputBuffer, 1, inputSize, input);
    fclose(input);

    uint8_t* output = (uint8_t*) malloc(inputSize);
    size_t   outputSize = 0;
    size_t   outputCapacity = inputSize;

    uint8_t* start = inputBuffer;
    uint8_t* end = inputBuffer + mem_find(inputBuffer, inputSize, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());
    size_t   length = end - start;

    uint8_t* limit = inputBuffer + inputSize;

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
        size_t inputStartIndex; // At which index the template starts in the input (for providing more information when an exception occurs).

        TemplateStackInfo(TemplateType typ, size_t outEnd, size_t inStart) : type(typ), outputEndIndex(outEnd), inputStartIndex(inStart) { };
    };

    std::stack<TemplateStackInfo> templateStack;

    while(end < limit) {
        if(start != end) {
            LOG_DEBUG("--> Found plaintext at %zu\n", start - inputBuffer);

            while(outputSize + 1 + OSH_PLAINTEXT_MARKER_LENGTH + 4 + length > outputCapacity) {
                outputCapacity += REALLOC_STEP_SIZE;
                output = (uint8_t*) realloc(output, outputCapacity);
            }

            LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
            outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_MARKER_LENGTH, start, length);
            LOG_DEBUG("done\n\n");
        }
        templateStartIndex = end - inputBuffer;

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
            remainingLength = inputSize - (end - inputBuffer);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected EOF", "did you forget to close the template?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            }

            if(index == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected template end", "did you forget to write the condition?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;
            ++end;

            if(start != end) { // Template conditional start.
                length = end - start;

                while(outputSize + 1 + OSH_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH + 4 + length + OSH_FORMAT > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template conditional start as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_TEMPLATE_CONDITIONAL_START_MARKER, OSH_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH, start, length);
                memset(output + outputSize, 0, OSH_FORMAT);
                
                templateStack.push(TemplateStackInfo(TemplateType::CONDITIONAL, outputSize, templateStartIndex));
                outputSize += OSH_FORMAT;
                LOG_DEBUG("done\n\n");
            }

            end = start;
        } else if(membcmp(end, Options::getTemplateConditionalEnd(), Options::getTemplateConditionalEndLength())) {
            LOG_DEBUG("Detected template conditional end\n");

            start = end;
            remainingLength = inputSize - (end - inputBuffer);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);       

                CompilationException exception("Unexpected EOF", "did you forget to close the template?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;

            if(start == end) { // Template conditional end.
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - inputBuffer;

                    mem_lncol(inputBuffer, errorIndex, &ln, &col);
                    uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);   

                    CompilationException exception("Unexpected conditional end", "there is no conditional to close; delete this", ln, col, chunk, chunkIndex, chunkSize);

                    free(chunk);
                    free(output);
                    free(inputBuffer);

                    throw exception;
                }
                if(templateStack.top().type != TemplateType::CONDITIONAL) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - inputBuffer;

                    mem_lncol(inputBuffer, templateStack.top().inputStartIndex, &ln, &col);

                    std::string msgBuffer;
                    msgBuffer.reserve(64);

                    msgBuffer += "close the";          

                    switch(templateStack.top().type) {
                        case TemplateType::LOOP:
                            msgBuffer += " loop ";
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

                    mem_lncol(inputBuffer, errorIndex, &ln, &col);
                    uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);

                    CompilationException exception("Unexpected conditional end", msgBuffer.c_str(), ln, col, chunk, chunkIndex, chunkSize);

                    free(chunk);
                    free(output);
                    free(inputBuffer);

                    throw exception;
                }

                ++end;
                length = 0;

                while(outputSize + 1 + OSH_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template conditional end as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_TEMPLATE_CONDITIONAL_END_MARKER, OSH_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH, start, length);
                memcpy(output + templateStack.top().outputEndIndex, &outputSize, OSH_FORMAT);
                
                templateStack.pop();
                LOG_DEBUG("done\n\n");

                end = start;
            } else {
                LOG_DEBUG("Re-detected as ordinary template\n");

                ++end;
                length = end - start;

                while(outputSize + 1 + OSH_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, start, length);
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

            remainingLength = inputSize - (end - inputBuffer);
            sepIndex = mem_find(end, remainingLength, Options::getTemplateLoopSeparator(), Options::getTemplateLoopSeparatorLength(), Options::getTemplateLoopSeparatorLookup());
            index    = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(sepIndex > index) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected end of template", "did you forget to write the loop separator?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            } else if(sepIndex == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + sepIndex) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected EOF", "did you forget to write the loop separator?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            } else if(sepIndex == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - inputBuffer;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected separator", "did you forget to provide the left argument before the separator?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            } else if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected EOF", "did you forget to close the template?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            }

            LOG_DEBUG("Found template loop separator at %zu\n", end + sepIndex - inputBuffer);
            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

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
                size_t errorIndex = (leftStart + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected end of template", "did you forget to provide the right argument after the separator?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            }

            while(*start == ' ' || *start == '\t')
                ++start;

            length = end - start;
            size_t leftLength = leftEnd - leftStart;

            while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_LOOP_START_MARKER_LENGTH > outputCapacity) {
                outputCapacity += REALLOC_STEP_SIZE;
                output = (uint8_t*) realloc(output, outputCapacity);
            }

            LOG_DEBUG("Writing template loop start as BDP832 pair %zu -> %zu...", leftStart - inputBuffer, end - inputBuffer);
            outputSize += BDP::writeName(Global::BDP832, output + outputSize, OSH_TEMPLATE_LOOP_START_MARKER, OSH_TEMPLATE_LOOP_START_MARKER_LENGTH);

            size_t tempBufferSize = Global::BDP832->VALUE_LENGTH_BYTE_SIZE * 2 + leftLength + length;
            uint8_t* tempBuffer = (uint8_t*) malloc(tempBufferSize);

            BDP::writeValue(Global::BDP832, tempBuffer, leftStart, leftLength);
            BDP::writeValue(Global::BDP832, tempBuffer + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength, start, length);

            while(outputSize + tempBufferSize + OSH_FORMAT > outputCapacity) {
                outputCapacity += REALLOC_STEP_SIZE;
                output = (uint8_t*) realloc(output, outputCapacity);
            }

            outputSize += BDP::writeValue(Global::BDP832, output + outputSize, tempBuffer, tempBufferSize);
            memset(output + outputSize, 0, OSH_FORMAT);
                
            templateStack.push(TemplateStackInfo(TemplateType::LOOP, outputSize, templateStartIndex));
            outputSize += OSH_FORMAT;
            free(tempBuffer);

            LOG_DEBUG("done\n\n");

            end = leftStart;
        } else if(membcmp(end, Options::getTemplateLoopEnd(), Options::getTemplateLoopEndLength())) {
            LOG_DEBUG("Detected template loop end\n");

            start = end;
            remainingLength = inputSize - (end - inputBuffer);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected EOF", "did you forget to close the template?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;

            if(start == end) { // Template loop end.
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - inputBuffer;

                    mem_lncol(inputBuffer, errorIndex, &ln, &col);
                    uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                    CompilationException exception("Unexpected loop end", "there is no loop to close; delete this", ln, col, chunk, chunkIndex, chunkSize);

                    free(chunk);
                    free(output);
                    free(inputBuffer);

                    throw exception;
                }
                if(templateStack.top().type != TemplateType::LOOP) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - inputBuffer;

                    mem_lncol(inputBuffer, templateStack.top().inputStartIndex, &ln, &col);

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

                    mem_lncol(inputBuffer, errorIndex, &ln, &col);
                    uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);

                    CompilationException exception("Unexpected loop end", msgBuffer.c_str(), ln, col, chunk, chunkIndex, chunkSize);

                    free(chunk);
                    free(output);
                    free(inputBuffer);

                    throw exception;
                }

                ++end;
                length = 0;

                if(outputSize + 1 + OSH_TEMPLATE_LOOP_END_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template loop end as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_TEMPLATE_LOOP_END_MARKER, OSH_TEMPLATE_LOOP_END_MARKER_LENGTH, start, length);
                memcpy(output + templateStack.top().outputEndIndex, &outputSize, OSH_FORMAT);
                
                templateStack.pop();
                LOG_DEBUG("done\n\n");

                end = start;
            } else {
                LOG_DEBUG("Re-detected as ordinary template\n");

                ++end;
                length = end - start;

                if(outputSize + 1 + OSH_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n\n");

                end = start;
            }
        } else if(membcmp(end, Options::getTemplateComponent(), Options::getTemplateComponentLength())) {
            LOG_DEBUG("Detected template component\n");

            end += Options::getTemplateComponentLength();

            while((*end == ' ' || *end == '\t') && end < inputBuffer + inputSize)
                ++end;

            start = end;

            uint8_t* leftStart = start;
            uint8_t* leftEnd = start;
            size_t   sepIndex;

            remainingLength = inputSize - (end - inputBuffer);
            sepIndex = mem_find(end, remainingLength, Options::getTemplateComponentSeparator(), Options::getTemplateComponentSeparatorLength(), Options::getTemplateComponentSeparatorLookup());
            index    = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(sepIndex == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - inputBuffer;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected separator", "did you forget to provide the component name before the separator?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            } else if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected EOF", "did you forget to close the template?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            } 
            
            if(index == 0) {
                LOG_DEBUG("Detected empty template component\n\n");
            } else {
                LOG_DEBUG("Found template component at %zu\n", end + sepIndex - inputBuffer);
                LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

                templateEndIndex = end + index - inputBuffer;

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
                        size_t errorIndex = (leftStart + index) - inputBuffer - 1;

                        mem_lncol(inputBuffer, errorIndex, &ln, &col);
                        uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                        CompilationException exception("Unexpected end of template", "did you forget to provide the component context after the separator?", ln, col, chunk, chunkIndex, chunkSize);

                        free(chunk);
                        free(output);
                        free(inputBuffer);

                        throw exception;
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
                // Here.

                bool isSelf = false;
                uint8_t* selfStart;

                if(end - start + 1 >= Options::getTemplateComponentSelfLength()) {
                    selfStart = end - Options::getTemplateComponentSelfLength();
                    if(0 == mem_find(selfStart, inputSize - (selfStart - inputBuffer), Options::getTemplateComponentSelf(), Options::getTemplateComponentSelfLength(), Options::getTemplateComponentSelfLookup())) {
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
                    size_t errorIndex = (selfStart) - inputBuffer;

                    mem_lncol(inputBuffer, errorIndex, &ln, &col);
                    uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                    CompilationException exception("Unexpected end of template", "did you forget to provide the component context after the separator?", ln, col, chunk, chunkIndex, chunkSize);

                    free(chunk);
                    free(output);
                    free(inputBuffer);

                    throw exception;
                }

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_COMPONENT_MARKER_LENGTH > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template component as BDP832 pair %zu -> %zu...", leftStart - inputBuffer, end - inputBuffer);
                outputSize += BDP::writeName(Global::BDP832, output + outputSize, OSH_TEMPLATE_COMPONENT_MARKER, OSH_TEMPLATE_COMPONENT_MARKER_LENGTH);

                size_t tempBufferSize = Global::BDP832->VALUE_LENGTH_BYTE_SIZE * 2 + leftLength + length;
                uint8_t* tempBuffer = (uint8_t*) malloc(tempBufferSize);

                BDP::writeValue(Global::BDP832, tempBuffer, leftStart, leftLength);
                BDP::writeValue(Global::BDP832, tempBuffer + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength, start, length);

                while(outputSize + tempBufferSize + OSH_FORMAT > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                outputSize += BDP::writeValue(Global::BDP832, output + outputSize, tempBuffer, tempBufferSize);
                memset(output + outputSize, 0, OSH_FORMAT);
                
                if(!isSelf)
                    templateStack.push(TemplateStackInfo(TemplateType::COMPONENT,outputSize, templateStartIndex));
                outputSize += OSH_FORMAT;
                free(tempBuffer);

                LOG_DEBUG("done\n\n");

                end = leftStart;
            }
        } else if(membcmp(end, Options::getTemplateComponentEnd(), Options::getTemplateComponentEndLength())) {
            LOG_DEBUG("Detected template component end\n");

            start = end;
            remainingLength = inputSize - (end - inputBuffer);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected EOF", "did you forget to close the template?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;

            if(start == end) { // Template component end.
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - inputBuffer;

                    mem_lncol(inputBuffer, errorIndex, &ln, &col);
                    uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                    CompilationException exception("Unexpected component end", "there is no component to close; delete this", ln, col, chunk, chunkIndex, chunkSize);

                    free(chunk);
                    free(output);
                    free(inputBuffer);

                    throw exception;
                }
                if(templateStack.top().type != TemplateType::COMPONENT) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - inputBuffer;

                    mem_lncol(inputBuffer, templateStack.top().inputStartIndex, &ln, &col);

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

                    mem_lncol(inputBuffer, errorIndex, &ln, &col);
                    uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);

                    CompilationException exception("Unexpected component end", msgBuffer.c_str(), ln, col, chunk, chunkIndex, chunkSize);

                    free(chunk);
                    free(output);
                    free(inputBuffer);

                    throw exception;
                }

                ++end;
                length = 0;

                while(outputSize + 1 + OSH_TEMPLATE_COMPONENT_END_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template component end as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_TEMPLATE_COMPONENT_END_MARKER, OSH_TEMPLATE_COMPONENT_END_MARKER_LENGTH, start, length);
                memcpy(output + templateStack.top().outputEndIndex, &outputSize, OSH_FORMAT);
                
                templateStack.pop();
                LOG_DEBUG("done\n\n");

                end = start;
            } else {
                LOG_DEBUG("Re-detected as ordinary template\n");

                ++end;
                length = end - start;

                while(outputSize + 1 + OSH_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n\n");

                end = start;
            }
        } else { // Template.
            start = end;
            remainingLength = inputSize - (end - inputBuffer);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - inputBuffer - 1;

                mem_lncol(inputBuffer, errorIndex, &ln, &col);
                uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);            

                CompilationException exception("Unexpected EOF", "did you forget to close the template?", ln, col, chunk, chunkIndex, chunkSize);

                free(chunk);
                free(output);
                free(inputBuffer);

                throw exception;
            } else if(index != 0) {
                LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

                end = end + index - 1;
                while(*end == ' ' || *end == '\t')
                    --end;
                ++end;

                length = end - start;

                if(outputSize + 1 + OSH_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n\n");

                end = start;
            } else {
                LOG_DEBUG("Detected empty template\n\n");
            }
        }

        start = end + index + Options::getTemplateEndLength();

        if(start >= limit)
            break;

        remainingLength = inputSize - (start - inputBuffer);
        end = start + mem_find(start, remainingLength, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());
        length = end - start;
    }

    if(!templateStack.empty()) {
        size_t ln;
        size_t col;
        size_t chunkIndex;
        size_t chunkSize;
        size_t errorIndex = templateStack.top().inputStartIndex;

        mem_lncol(inputBuffer, errorIndex, &ln, &col);
        uint8_t* chunk = mem_lnchunk(inputBuffer, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize);

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

        CompilationException exception(msgBuffer.c_str(), "did you forget to close this?", ln, col, chunk, chunkIndex, chunkSize);

        free(chunk);
        free(output);
        free(inputBuffer);

        throw exception;
    }

    if(end > start) {
        LOG_DEBUG("--> Found plaintext at %zu\n", start - inputBuffer);

        if(outputSize + 1 + OSH_PLAINTEXT_MARKER_LENGTH + 4 + length > outputCapacity) {
            outputCapacity += REALLOC_STEP_SIZE;
            output = (uint8_t*) realloc(output, outputCapacity);
        }

        LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
        outputSize += BDP::writePair(Global::BDP832, output + outputSize, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_MARKER_LENGTH, start, length);
        LOG_DEBUG("done\n\n")
    }

    if(outputSize != outputCapacity)
        output = (uint8_t*) realloc(output, outputSize);

    LOG_DEBUG("Wrote %zd bytes to output\n", outputSize)

    free(inputBuffer);

    FILE* dest = fopen(outputPath, "wb");
    fwrite(output, 1, outputSize, dest);
    fclose(dest);

    free(output);
}
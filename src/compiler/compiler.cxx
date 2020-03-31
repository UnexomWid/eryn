#pragma warning(disable : 4996)

#include "compiler.hxx"
#include "../../lib/bdp.hxx"
#include "../../lib/mem_find.h"
#include "../global/cache.hxx"
#include "../global/global.hxx"
#include "../global/options.hxx"

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

    if(input == NULL) {
        throw std::runtime_error((std::string("Cannot open file '") + path) + "'\n");
    }

    fseek(input, 0, SEEK_END);
    long inputSize = ftell(input);
    fseek(input, 0, SEEK_SET);

    LOG_DEBUG("File size is %ld bytes\n", inputSize);

    uint8_t* inputBuffer = (uint8_t*) malloc(inputSize);

    fread(inputBuffer, 1, inputSize, input);
    fclose(input);

    uint8_t* output = (uint8_t*) malloc(inputSize);
    size_t outputSize = 0;
    size_t outputCapacity = inputSize;

    uint8_t* start = inputBuffer;
    uint8_t* end = inputBuffer + mem_find(inputBuffer, inputSize, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());
    size_t length = end - start;

    uint8_t* limit = inputBuffer + inputSize;

    size_t remainingLength;
    size_t index;
    size_t templateStartIndex;

    while(end < limit) {
        if(start != end) {
            LOG_DEBUG("\n--> Found plaintext at %zu\n", start - inputBuffer);
            if(outputSize + 1 + COMPILER_PLAINTEXT_MARKER_LENGTH + 4 + length > outputCapacity) {
                outputCapacity += REALLOC_STEP_SIZE;
                output = (uint8_t*) realloc(output, outputCapacity);
            }
            LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
            outputSize += BDP::writePair(Global::BDP832, output + outputSize, COMPILER_PLAINTEXT_MARKER, COMPILER_PLAINTEXT_MARKER_LENGTH, start, length);
            LOG_DEBUG("done\n");
        }
        templateStartIndex = end - inputBuffer;

        LOG_DEBUG("\n--> Found template start at %zu\n", templateStartIndex);

        end += Options::getTemplateStartLength();

        while(*end == ' ' || *end == '\t')
            ++end;

        // Template conditional start.
        if(membcmp(end, Options::getTemplateConditionalStart(), Options::getTemplateConditionalStartLength())) {
            LOG_DEBUG("Detected template conditional start\n");
            end += Options::getTemplateConditionalStartLength();

            while(*end == ' ' || *end == '\t')
                ++end;

            start = end;

            remainingLength = inputSize - (end - inputBuffer);

            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                throw std::runtime_error("Unexpected EOF: did you forget to close the template?");
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;
            ++end;

            if(start != end) { // Template conditional start.
                length = end - start;

                if(outputSize + 1 + COMPILER_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }
                LOG_DEBUG("Writing template conditional start as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, COMPILER_TEMPLATE_CONDITIONAL_START_MARKER, COMPILER_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n");
            }

            end = start;
        } else if(membcmp(end, Options::getTemplateConditionalEnd(), Options::getTemplateConditionalEndLength())) {
            LOG_DEBUG("Detected template conditional end\n");
            start = end;

            remainingLength = inputSize - (end - inputBuffer);

            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                throw std::runtime_error("Unexpected EOF: did you forget to close the template?");
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;

            if(start == end) { // Template conditional end.
                ++end;
                length = 0;

                if(outputSize + 1 + COMPILER_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }
                LOG_DEBUG("Writing template conditional end as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, COMPILER_TEMPLATE_CONDITIONAL_END_MARKER, COMPILER_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n");

                end = start;
            } else { // Template.
                LOG_DEBUG("Re-detected as ordinary template\n");
                ++end;
                length = end - start;

                if(outputSize + 1 + COMPILER_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }
                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, COMPILER_TEMPLATE_MARKER, COMPILER_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n");

                end = start;
            }
        } if(membcmp(end, Options::getTemplateLoopStart(), Options::getTemplateLoopStartLength())) {
            LOG_DEBUG("Detected template loop start\n");
            end += Options::getTemplateLoopStartLength();

            while(*end == ' ' || *end == '\t')
                ++end;

            start = end;

            uint8_t* leftStart = start;
            uint8_t* leftEnd = start;
            size_t sepIndex;

            remainingLength = inputSize - (end - inputBuffer);

            sepIndex = mem_find(end, remainingLength, Options::getTemplateLoopSeparator(), Options::getTemplateLoopSeparatorLength(), Options::getTemplateLoopSeparatorLookup());

            if(sepIndex == remainingLength) {
                throw std::runtime_error("Unexpected EOF: did you forget to write the loop separator?");
            } else if(sepIndex == 0) {
                throw std::runtime_error("Unexpected separator: did you forget to provide the left argument before the separator?");
            }

            LOG_DEBUG("Found template loop separator at %zu\n", end + sepIndex - inputBuffer);

            leftEnd = end + sepIndex - 1;
            start = end + sepIndex + Options::getTemplateLoopSeparatorLength();

            while(*leftEnd == ' ' || *leftEnd == '\t')
                --leftEnd;
            ++leftEnd;

            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                throw std::runtime_error("Unexpected EOF: did you forget to close the template?");
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;
            ++end;

            if(end == start) {
                throw std::runtime_error("Unexpected end: did you forget to provide the right argument after the separator?");
            }

            while(*start == ' ' || *start == '\t')
                ++start;

            length = end - start;
            size_t leftLength = leftEnd - leftStart;

            while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + COMPILER_TEMPLATE_LOOP_START_MARKER_LENGTH > outputCapacity) {
                outputCapacity += REALLOC_STEP_SIZE;
                output = (uint8_t*) realloc(output, outputCapacity);
            }

            LOG_DEBUG("Writing template loop start as BDP832 pair %zu -> %zu...", leftStart - inputBuffer, end - inputBuffer);
            outputSize += BDP::writeName(Global::BDP832, output + outputSize, COMPILER_TEMPLATE_LOOP_START_MARKER, COMPILER_TEMPLATE_LOOP_START_MARKER_LENGTH);

            size_t tempBufferSize = Global::BDP832->VALUE_LENGTH_BYTE_SIZE * 2 + leftLength + length;
            uint8_t* tempBuffer = (uint8_t*) malloc(tempBufferSize);

            BDP::writeValue(Global::BDP832, tempBuffer, leftStart, leftLength);
            BDP::writeValue(Global::BDP832, tempBuffer + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength, start, length);

            while(outputSize + tempBufferSize > outputCapacity) {
                outputCapacity += REALLOC_STEP_SIZE;
                output = (uint8_t*) realloc(output, outputCapacity);
            }

            outputSize += BDP::writeValue(Global::BDP832, output + outputSize, tempBuffer, tempBufferSize);
            free(tempBuffer);
            LOG_DEBUG("done\n");

            end = leftStart;
        } else if(membcmp(end, Options::getTemplateLoopEnd(), Options::getTemplateLoopEndLength())) {
            LOG_DEBUG("Detected template loop end\n");
            start = end;

            remainingLength = inputSize - (end - inputBuffer);

            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                throw std::runtime_error("Unexpected EOF: did you forget to close the template?");
            }

            LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

            end = end + index - 1;
            while(*end == ' ' || *end == '\t')
                --end;

            if(start == end) { // Template loop end.
                ++end;
                length = 0;

                if(outputSize + 1 + COMPILER_TEMPLATE_LOOP_END_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }
                LOG_DEBUG("Writing template loop end as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, COMPILER_TEMPLATE_LOOP_END_MARKER, COMPILER_TEMPLATE_LOOP_END_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n");

                end = start;
            } else { // Template.
                LOG_DEBUG("Re-detected as ordinary template\n");
                ++end;
                length = end - start;

                if(outputSize + 1 + COMPILER_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }
                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, COMPILER_TEMPLATE_MARKER, COMPILER_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n");

                end = start;
            }
        } else { // Template.
            start = end;

            remainingLength = inputSize - (end - inputBuffer);

            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            if(index == remainingLength) {
                throw std::runtime_error("Unexpected EOF: did you forget to close the template?");
            } else if(index != 0) {
                LOG_DEBUG("Found template end at %zu\n", end + index - inputBuffer);

                end = end + index - 1;
                while(*end == ' ' || *end == '\t')
                    --end;
                ++end;

                length = end - start;

                if(outputSize + 1 + COMPILER_TEMPLATE_MARKER_LENGTH + 4 + length > outputCapacity) {
                    outputCapacity += REALLOC_STEP_SIZE;
                    output = (uint8_t*) realloc(output, outputCapacity);
                }
                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
                outputSize += BDP::writePair(Global::BDP832, output + outputSize, COMPILER_TEMPLATE_MARKER, COMPILER_TEMPLATE_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n");

                end = start;
            } else {
                LOG_DEBUG("Detected empty template\n");
            }
        }

        start = end + index + Options::getTemplateEndLength();

        if(start >= limit)
            break;

        remainingLength = inputSize - (start - inputBuffer);

        end = start + mem_find(start, remainingLength, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());

        length = end - start;
    }

    if(end > start) {
        LOG_DEBUG("\n--> Found plaintext at %zu\n", start - inputBuffer);
        if(outputSize + 1 + COMPILER_PLAINTEXT_MARKER_LENGTH + 4 + length > outputCapacity) {
            outputCapacity += REALLOC_STEP_SIZE;
            output = (uint8_t*) realloc(output, outputCapacity);
        }
        LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - inputBuffer, end - inputBuffer);
        outputSize += BDP::writePair(Global::BDP832, output + outputSize, COMPILER_PLAINTEXT_MARKER, COMPILER_PLAINTEXT_MARKER_LENGTH, start, length);
        LOG_DEBUG("done\n")
    }

    if(outputSize != outputCapacity)
        output = (uint8_t*) realloc(output, outputSize);

    LOG_DEBUG("\nWrote %zd bytes to output\n", outputSize)

    free(inputBuffer);

    FILE* dest = fopen(outputPath, "wb");
    fwrite(output, 1, outputSize, dest);
    fclose(dest);
    free(output);
}
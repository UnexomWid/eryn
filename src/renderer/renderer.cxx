#include "renderer.hxx"

#include "../def/osh.dxx"
#include "../def/logging.dxx"

#include "../../lib/bdp.hxx"
#include "../../lib/mem_find.h"

#include "../global/cache.hxx"
#include "../global/global.hxx"
#include "../global/options.hxx"

#include "../except/rendering.hxx"

#include <stack>
#include <cstdio>
#include <memory>

using Global::Cache;
using Global::Options;

BinaryData render(BridgeData data, const char* path) {
    LOG_INFO("===> Rendering '%s'", path);

    if(!Cache::hasEntry(path))
        throw RenderingException("Item does not exist in cache", "did you forget to compile this file?");

    BinaryData entry = Cache::getEntry(path);
    return renderBytes(data, entry.data, entry.size);
}

void renderFile(BridgeData data, const char* path, const char* outputPath) {
    LOG_INFO("===> Rendering file '%s'", path);

    FILE* input = fopen(path, "rb");

    if(input == NULL)
        throw RenderingException("Render error", "cannot open input file");

    fseek(input, 0, SEEK_END);
    long fileLength = ftell(input);
    fseek(input, 0, SEEK_SET);

    LOG_DEBUG("File size is %ld bytes\n", fileLength);

    size_t inputSize = (size_t) fileLength;
    std::unique_ptr<uint8_t, decltype(qfree)*> inputBuffer(qmalloc(inputSize), qfree);

    fread(inputBuffer.get(), 1, inputSize, input);
    fclose(input);

    BinaryData rendered = renderBytes(data, inputBuffer.get(), inputSize);

    LOG_DEBUG("Wrote %zd bytes to output\n", rendered.size)

    FILE* dest = fopen(outputPath, "wb");
    fwrite(rendered.data, 1, rendered.size, dest);
    fclose(dest);

    qfree((uint8_t*) rendered.data);
}

void renderComponent(BridgeData data, const uint8_t* component, size_t componentSize, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity, const uint8_t* content, size_t contentSize, const uint8_t* parentContent, size_t parentContentSize) {
    std::string path(reinterpret_cast<const char*>(component), componentSize);

    LOG_INFO("===> Rendering component '%s'", path.c_str());

    if(!Cache::hasEntry(path))
        throw RenderingException("Item does not exist in cache", "did you forget to compile this file?");

    BinaryData entry = Cache::getEntry(path);
    renderBytes(data, entry.data, entry.size, output, outputSize, outputCapacity, content, contentSize, parentContent, parentContentSize);

    LOG_DEBUG("===> Done\n");
}

BinaryData renderBytes(BridgeData data, const uint8_t* input, const size_t inputSize) {
    return renderBytes(data, input, inputSize, nullptr, 0, nullptr, 0);
}

BinaryData renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, const uint8_t* content, size_t contentSize, const uint8_t* parentContent, size_t parentContentSize) {
    size_t outputSize = 0;
    size_t outputCapacity = inputSize;
    std::unique_ptr<uint8_t, decltype(qfree)*> output(qalloc(outputCapacity), qfree);

    renderBytes(data, input, inputSize, output, outputSize, outputCapacity, content, contentSize, parentContent, parentContentSize);

    // Bring the capacity to the actual size.
    if(outputSize != outputCapacity) {
        uint8_t* newBuffer = qrealloc(output.get(), outputSize);
        output.release();
        output.reset(newBuffer);
    }

    uint8_t* rendered = output.get();
    output.release();

    return BinaryData(rendered, outputSize);
}

void renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity, const uint8_t* content, size_t contentSize, const uint8_t* parentContent, size_t parentContentSize) {
    size_t inputIndex  = 0;
    size_t nameLength  = 0;
    size_t valueLength = 0;

    const uint8_t* name;
    const uint8_t* value;

    struct LoopStackInfo {
        size_t arrayIndex;              // Used to iterate over the array.
        size_t arrayLength;             // Used to stop the iteration.
        size_t assignmentUpdateIndex;   // Used to update the assignment string.
        size_t assignmentUnassignIndex; // Used to unassign the variable that the assignment string creates.

        std::string assignment;         // Used to assign a value from the array to a variable.

        LoopStackInfo(BridgeData data, const uint8_t* iterator, size_t iteratorSize, const uint8_t* array, size_t arraySize) : arrayIndex(0), arrayLength(getArrayLength(data, array, arraySize)) {
            buildLoopAssignment(data, assignment, assignmentUpdateIndex, assignmentUnassignIndex, iterator, iteratorSize, array, arraySize);
            update();
        };

        void invalidate() { invalidateLoopAssignment(assignment, assignmentUpdateIndex); }
        void update() { updateLoopAssignment(assignment, arrayIndex); }
    };

    std::stack<LoopStackInfo> loopStack;

    while(inputIndex < inputSize) {
        if(BDP::isLittleEndian()) {
            BDP::directBytesToLength(nameLength, input + inputIndex, Global::BDP832->NAME_LENGTH_BYTE_SIZE);
            inputIndex += Global::BDP832->NAME_LENGTH_BYTE_SIZE;
            name = input + inputIndex;
            inputIndex += nameLength;

            BDP::directBytesToLength(valueLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
            inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
            value = input + inputIndex;
            inputIndex += valueLength;
        } else {
            BDP::bytesToLength(nameLength, input + inputIndex, Global::BDP832->NAME_LENGTH_BYTE_SIZE);
            inputIndex += Global::BDP832->NAME_LENGTH_BYTE_SIZE;
            name = input + inputIndex;
            input += nameLength;

            BDP::bytesToLength(valueLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
            inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
            value = input + inputIndex;
            input += valueLength;
        }

        uint8_t nameByte = *name;
        
        // Only compares the first byte, for performance reasons. Change this if the markers have length > 1.
        if(nameByte == *OSH_PLAINTEXT_MARKER) {
            LOG_DEBUG("--> Found plaintext");

            while(outputSize + valueLength > outputCapacity) {
                uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                output.release();
                output.reset(newOutput);
            }

            memcpy(output.get() + outputSize, value, valueLength);
            outputSize += valueLength;
        } else if(nameByte == *OSH_TEMPLATE_MARKER) {
            LOG_DEBUG("--> Found template");

            if(valueLength == OSH_TEMPLATE_CONTENT_MARKER_LENGTH && membcmp(value, OSH_TEMPLATE_CONTENT_MARKER, valueLength)) {
                if(contentSize == 0u) {
                    if(Options::getThrowOnEmptyContent())
                        throw RenderingException("No content", "there is no content for this component");
                } else renderBytes(data, content, contentSize, output, outputSize, outputCapacity, parentContent, parentContentSize, nullptr, 0u);
            } else evalTemplate(data, value, valueLength, output, outputSize, outputCapacity);
        } else if(nameByte == *OSH_TEMPLATE_CONDITIONAL_START_MARKER) {
            LOG_DEBUG("--> Found conditional template start");

            size_t conditionalEnd;
            if(BDP::isLittleEndian())
                BDP::directBytesToLength(conditionalEnd, input + inputIndex, OSH_FORMAT);
            else BDP::bytesToLength(conditionalEnd, input + inputIndex, OSH_FORMAT);

            inputIndex += OSH_FORMAT;

            if(!evalConditionalTemplate(data, value, valueLength, output, outputSize, outputCapacity))
                inputIndex = conditionalEnd;
        } else if(nameByte == *OSH_TEMPLATE_CONDITIONAL_END_MARKER) {
            LOG_DEBUG("--> Found conditional template end");

            continue;
        } else if(nameByte == *OSH_TEMPLATE_LOOP_START_MARKER) {
            LOG_DEBUG("--> Found loop template start");

            size_t leftLength;
            size_t rightLength;

            const uint8_t* left;
            const uint8_t* right;

            inputIndex -= valueLength;

            if(BDP::isLittleEndian()) {
                left = input + inputIndex + Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                BDP::directBytesToLength(leftLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength;

                BDP::directBytesToLength(rightLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                right = input + inputIndex;
                inputIndex += rightLength;
            } else {
                left = input + inputIndex + Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                BDP::bytesToLength(leftLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength;

                BDP::bytesToLength(rightLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                right = input + inputIndex;
                inputIndex += rightLength;
            }

            loopStack.push(LoopStackInfo(data, left, leftLength, right, rightLength));

            if(loopStack.top().arrayIndex >= loopStack.top().arrayLength) {
                if(BDP::isLittleEndian()) {
                    BDP::directBytesToLength(inputIndex, input + inputIndex, OSH_FORMAT);
                } else BDP::bytesToLength(inputIndex, input + inputIndex, OSH_FORMAT);

                loopStack.pop();
            } else {
                inputIndex += OSH_FORMAT;
                evalAssignment(data, loopStack.top().assignment);
            }
        } else if(nameByte == *OSH_TEMPLATE_LOOP_END_MARKER) {
            LOG_DEBUG("--> Found loop template end");

            size_t returnIndex;

            if(BDP::isLittleEndian()) {
                BDP::directBytesToLength(returnIndex, input + inputIndex, OSH_FORMAT);
            } else BDP::bytesToLength(returnIndex, input + inputIndex, OSH_FORMAT);

            inputIndex += OSH_FORMAT;

            if(loopStack.top().arrayIndex < loopStack.top().arrayLength) {
                loopStack.top().invalidate();
                loopStack.top().update();

                evalAssignment(data, loopStack.top().assignment);
                inputIndex = returnIndex;
            } else {
                unassign(data, loopStack.top().assignment, loopStack.top().assignmentUnassignIndex);
                loopStack.pop();
            }
        } else if(nameByte == *OSH_TEMPLATE_COMPONENT_MARKER) {
            LOG_DEBUG("--> Found component template\n");

            size_t leftLength;
            size_t rightLength;

            const uint8_t* left;
            const uint8_t* right;

            inputIndex -= valueLength;

            if(BDP::isLittleEndian()) {
                left = input + inputIndex + Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                BDP::directBytesToLength(leftLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength;

                BDP::directBytesToLength(rightLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                right = input + inputIndex;
                inputIndex += rightLength;
            } else {
                left = input + inputIndex + Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                BDP::bytesToLength(leftLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength;

                BDP::bytesToLength(rightLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                right = input + inputIndex;
                inputIndex += rightLength;
            }

            size_t contentLength;

            if(BDP::isLittleEndian()) {
                BDP::directBytesToLength(contentLength, input + inputIndex, OSH_FORMAT);
            } else BDP::bytesToLength(contentLength, input + inputIndex, OSH_FORMAT);

            inputIndex += OSH_FORMAT;

            BridgeBackup contextBackup = backupContext(data);

            initContext(data, right, rightLength);
            renderComponent(data, left, leftLength, output, outputSize, outputCapacity, input + inputIndex, contentLength, content, contentSize);
            restoreContext(data, contextBackup);

            inputIndex += contentLength;
        } else if(nameByte == *OSH_TEMPLATE_COMPONENT_END_MARKER) {
            LOG_DEBUG("--> Found component template end");

            continue;
        } else throw RenderingException("Not dupported", "this template type is not supported");
    }
}
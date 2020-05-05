#include "renderer.hxx"

#include "../def/osh.dxx"
#include "../def/logging.dxx"

#include "../../lib/bdp.hxx"
#include "../../lib/mem_find.h"
#include "../../lib/timer.hxx"

#include "../global/cache.hxx"
#include "../global/global.hxx"
#include "../global/options.hxx"

#include "../except/rendering.hxx"

#include "../compiler/compiler.hxx"

#include <stack>
#include <cstdio>
#include <memory>

using Global::Cache;
using Global::Options;

BinaryData render(BridgeData data, const char* path) {
    LOG_DEBUG("===> Rendering '%s'", path);

    CHRONOMETER chrono = time_now();

    if(Options::getBypassCache()) {
        std::unordered_set<std::string> recompiled;

        compile(path);
        recompiled.insert(std::string(path));

        BinaryData entry = Cache::getEntry(path);

        if(Options::getLogRenderTime()) {
            BinaryData rendered = renderBytes(data, entry.data, entry.size, &recompiled);
            LOG_INFO("Rendered in %s\n", getf_exec_time_mis(chrono).c_str());
            return rendered;
        } else return renderBytes(data, entry.data, entry.size, &recompiled);
    } else if(!Cache::hasEntry(path)) {
        if(Options::getThrowOnMissingEntry())
            throw RenderingException("Item does not exist in cache", "did you forget to compile this?");

        compile(path);
        
        BinaryData entry = Cache::getEntry(path);

        if(Options::getLogRenderTime()) {
            BinaryData rendered = renderBytes(data, entry.data, entry.size, nullptr);
            LOG_INFO("Rendered in %s\n", getf_exec_time_mis(chrono).c_str());
            return rendered;
        } else return renderBytes(data, entry.data, entry.size, nullptr);
    } else {
        BinaryData entry = Cache::getEntry(path);

        if(Options::getLogRenderTime()) {
            BinaryData rendered = renderBytes(data, entry.data, entry.size, nullptr);
            LOG_INFO("Rendered in %s", getf_exec_time_mis(chrono).c_str());
            return rendered;
        } else return renderBytes(data, entry.data, entry.size, nullptr);
    }
}

void renderComponent(BridgeData data, const uint8_t* component, size_t componentSize, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity, const uint8_t* content, size_t contentSize, std::unordered_set<std::string>* recompiled) {
    std::string path(reinterpret_cast<const char*>(component), componentSize);

    LOG_DEBUG("===> Rendering component '%s'", path.c_str());

    if(Options::getBypassCache()) {
        if(recompiled->find(path) == recompiled->end()) {
            compile(path.c_str());
            recompiled->insert(std::string(path));
        }
    } else if(!Cache::hasEntry(path)) {
        if(Options::getThrowOnMissingEntry())
            throw RenderingException("Item does not exist in cache", "did you forget to compile this?");
        compile(path.c_str());
    }

    BinaryData entry = Cache::getEntry(path);
    renderBytes(data, entry.data, entry.size, output, outputSize, outputCapacity, content, contentSize, recompiled);

    LOG_DEBUG("===> Done\n");
}

BinaryData renderBytes(BridgeData data, const uint8_t* input, const size_t inputSize, std::unordered_set<std::string>* recompiled) {
    return renderBytes(data, input, inputSize, nullptr, 0, recompiled);
}

BinaryData renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, const uint8_t* content, size_t contentSize, std::unordered_set<std::string>* recompiled) {
    size_t outputSize = 0;
    size_t outputCapacity = inputSize;
    std::unique_ptr<uint8_t, decltype(qfree)*> output(qalloc(outputCapacity), qfree);

    renderBytes(data, input, inputSize, output, outputSize, outputCapacity, content, contentSize, recompiled);

    // Bring the capacity to the actual size.
    if(outputSize > 0 && outputSize != outputCapacity) {
        uint8_t* newBuffer = qrealloc(output.get(), outputSize);
        output.release();
        output.reset(newBuffer);
    }

    uint8_t* rendered = output.get();
    output.release();

    return BinaryData(rendered, outputSize);
}

void renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity, const uint8_t* content, size_t contentSize, std::unordered_set<std::string>* recompiled) {
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

    struct ComponentStackInfo {
        bool hasContent;

        const uint8_t* path;
        const uint8_t* context;

        size_t startIndex;
        size_t pathLength;
        size_t contextLength;
    };

    std::stack<LoopStackInfo>      loopStack;
    std::stack<ComponentStackInfo> componentStack;

    while(inputIndex < inputSize) {
        BDP::bytesToLength(nameLength, input + inputIndex, Global::BDP832->NAME_LENGTH_BYTE_SIZE);
        inputIndex += Global::BDP832->NAME_LENGTH_BYTE_SIZE;
        name = input + inputIndex;
        inputIndex += nameLength;

        BDP::bytesToLength(valueLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
        inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
        value = input + inputIndex;
        inputIndex += valueLength;

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
                        throw RenderingException("No content", "there is no content for this component", value, valueLength);
                } else {
                    memcpy(output.get() + outputSize, content, contentSize);
                    outputSize += contentSize;
                }
            } else evalTemplate(data, value, valueLength, output, outputSize, outputCapacity);
        } else if(nameByte == *OSH_TEMPLATE_CONDITIONAL_START_MARKER) {
            LOG_DEBUG("--> Found conditional template start");

            size_t conditionalEnd;
            BDP::bytesToLength(conditionalEnd, input + inputIndex, OSH_FORMAT);

            inputIndex += OSH_FORMAT;

            if(!evalConditionalTemplate(data, value, valueLength, output, outputSize, outputCapacity))
                inputIndex += conditionalEnd;
        } else if(nameByte == *OSH_TEMPLATE_CONDITIONAL_END_MARKER) {
            LOG_DEBUG("--> Found conditional template end");

            continue;
        } else if(nameByte == *OSH_TEMPLATE_INVERTED_CONDITIONAL_START_MARKER) {
            LOG_DEBUG("--> Found inverted conditional template start");

            size_t conditionalEnd;
            BDP::bytesToLength(conditionalEnd, input + inputIndex, OSH_FORMAT);

            inputIndex += OSH_FORMAT;

            if(evalConditionalTemplate(data, value, valueLength, output, outputSize, outputCapacity))
                inputIndex += conditionalEnd;
        } else if(nameByte == *OSH_TEMPLATE_INVERTED_CONDITIONAL_END_MARKER) {
            LOG_DEBUG("--> Found inverted conditional template end");

            continue;
        } else if(nameByte == *OSH_TEMPLATE_LOOP_START_MARKER) {
            LOG_DEBUG("--> Found loop template start");

            size_t leftLength;
            size_t rightLength;

            const uint8_t* left;
            const uint8_t* right;

            inputIndex -= valueLength;

            left = input + inputIndex + Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                BDP::bytesToLength(leftLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength;

                BDP::bytesToLength(rightLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
                inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
                right = input + inputIndex;
                inputIndex += rightLength;

            loopStack.push(LoopStackInfo(data, left, leftLength, right, rightLength));

            if(loopStack.top().arrayIndex >= loopStack.top().arrayLength) {
                size_t loopEnd;
                BDP::bytesToLength(loopEnd, input + inputIndex, OSH_FORMAT);

                inputIndex += OSH_FORMAT + loopEnd;
                loopStack.pop();
            } else {
                inputIndex += OSH_FORMAT;
                evalAssignment(data, loopStack.top().assignment);
            }
        } else if(nameByte == *OSH_TEMPLATE_LOOP_END_MARKER) {
            LOG_DEBUG("--> Found loop template end");

            if(loopStack.top().arrayIndex < loopStack.top().arrayLength) {
                loopStack.top().invalidate();
                loopStack.top().update();

                evalAssignment(data, loopStack.top().assignment);

                size_t loopStart;

                BDP::bytesToLength(loopStart, input + inputIndex, OSH_FORMAT);

                inputIndex += OSH_FORMAT;
                inputIndex -= loopStart;
            } else {
                inputIndex += OSH_FORMAT;

                unassign(data, loopStack.top().assignment, loopStack.top().assignmentUnassignIndex);
                loopStack.pop();
            }
        } else if(nameByte == *OSH_TEMPLATE_COMPONENT_MARKER) {
            LOG_DEBUG("--> Found component template\n");

            inputIndex -= valueLength;

            ComponentStackInfo info;

            info.path = input + inputIndex + Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
            BDP::bytesToLength(info.pathLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
            inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE + info.pathLength;

            BDP::bytesToLength(info.contextLength, input + inputIndex, Global::BDP832->VALUE_LENGTH_BYTE_SIZE);
            inputIndex += Global::BDP832->VALUE_LENGTH_BYTE_SIZE;
            info.context = input + inputIndex;
            inputIndex += info.contextLength;

            size_t contentLength;

            BDP::bytesToLength(contentLength, input + inputIndex, OSH_FORMAT);

            inputIndex += OSH_FORMAT;

            if(contentLength == 0) {
                info.hasContent = false;

                BridgeBackup contextBackup = backupContext(data);

                initContext(data, info.context, info.contextLength);
                renderComponent(data, info.path, info.pathLength, output, outputSize, outputCapacity, nullptr, 0, recompiled);
                restoreContext(data, contextBackup);
            } else {
                info.hasContent = true;
                info.startIndex = outputSize;
            }

            componentStack.push(info);
        } else if(nameByte == *OSH_TEMPLATE_COMPONENT_END_MARKER) {
            LOG_DEBUG("--> Found component template end");

            ComponentStackInfo info = componentStack.top();

            if(info.hasContent) {
                size_t contentLength = outputSize - info.startIndex;
                std::unique_ptr<uint8_t, decltype(qfree)*> contentBuffer(qmalloc(contentLength), qfree);

                memcpy(contentBuffer.get(), output.get() + info.startIndex, contentLength);

                outputSize = info.startIndex;

                BridgeBackup contextBackup = backupContext(data);

                initContext(data, info.context, info.contextLength);
                renderComponent(data, info.path, info.pathLength, output, outputSize, outputCapacity, contentBuffer.get(), contentLength, recompiled);
                restoreContext(data, contextBackup);
            }

            componentStack.pop();
        } else throw RenderingException("Not supported", "this template type is not supported", name, nameLength);
    }
}
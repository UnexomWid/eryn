#include "renderer.hxx"
#include "../../lib/bdp.hxx"
#include "../../lib/mem_find.h"
#include "../def/osh.dxx"
#include "../def/logging.dxx"
#include "../global/cache.hxx"
#include "../global/global.hxx"
#include "../global/options.hxx"
#include "../except/rendering.hxx"

#include <cstdio>
#include <memory>

using Global::Cache;
using Global::Options;

void renderFile(BridgeData data, const char* path, const char* outputPath) {
    LOG_INFO("===> Rendering file '%s'\n", path);

    FILE* input = fopen(path, "rb");

    if(input == NULL)
        throw RenderingException("Render error", "cannot open input file");

    fseek(input, 0, SEEK_END);
    long fileLength = ftell(input);
    fseek(input, 0, SEEK_SET);

    LOG_DEBUG("File size is %ld bytes\n\n", fileLength);

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

BinaryData renderBytes(BridgeData data, uint8_t* input, size_t inputSize) {
    size_t outputSize = 0;
    size_t outputCapacity = inputSize;
    std::unique_ptr<uint8_t, decltype(qfree)*> output(qalloc(outputCapacity), qfree);

    size_t inputIndex  = 0;
    size_t nameLength  = 0;
    size_t valueLength = 0;

    std::unique_ptr<uint8_t, decltype(qfree)*> name(qmalloc(256u), qfree);
    std::unique_ptr<uint8_t, decltype(qfree)*> value(qalloc(outputCapacity), qfree);

    while(inputIndex < inputSize) {
        inputIndex += BDP::readPair(Global::BDP832, input + inputIndex, (uint8_t*)name.get(), (uint64_t*)&nameLength, (uint8_t*)value.get(), (uint64_t*)&valueLength);

        uint8_t nameByte = *name;
        
        // Only compares the first byte, for performance reasons. Change this if the markers have length > 1.
        if(nameByte == *OSH_PLAINTEXT_MARKER) {
            LOG_DEBUG("Found plaintext\n");
            while(outputSize + valueLength > outputCapacity)
                qexpand(output.get(), outputCapacity);
            memcpy(output.get() + outputSize, value.get(), valueLength);
            outputSize += valueLength;
        } else if(nameByte == *OSH_TEMPLATE_MARKER) {
            LOG_DEBUG("Found template\n");
            evalTemplate(data, value.get(), valueLength, output, outputSize, outputCapacity);
        } else if(nameByte == *OSH_TEMPLATE_CONDITIONAL_START_MARKER) {
            LOG_DEBUG("Found conditional template start\n");
            size_t conditionalEnd;
            if(BDP::isLittleEndian())
                BDP::directBytesToLength(conditionalEnd, input + inputIndex, OSH_FORMAT);
            else BDP::bytesToLength(conditionalEnd, input + inputIndex, OSH_FORMAT);

            inputIndex += 4;

            if(!evalConditionalTemplate(data, value.get(), valueLength, output, outputSize, outputCapacity))
                inputIndex = conditionalEnd;
        } else if(nameByte == *OSH_TEMPLATE_CONDITIONAL_END_MARKER) {
            LOG_DEBUG("Found conditional template end\n");
            continue;
        } else throw RenderingException("Not Implemented", "this template type is not implemented");
    }

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
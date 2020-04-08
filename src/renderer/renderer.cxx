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

void renderFile(const char* path, const char* outputPath) {
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

    BinaryData rendered = renderBytes(inputBuffer.get(), inputSize);

    LOG_DEBUG("Wrote %zd bytes to output\n", compiled.size)

    FILE* dest = fopen(outputPath, "wb");
    fwrite(rendered.data, 1, rendered.size, dest);
    fclose(dest);

    qfree((uint8_t*) rendered.data);
}

BinaryData renderBytes(uint8_t* input, size_t inputSize) {
    size_t outputSize = 0;
    size_t outputCapacity = inputSize;
    std::unique_ptr<uint8_t, decltype(qfree)*> output(qalloc(outputCapacity), qfree);

    size_t inputIndex  = 0;
    size_t nameLength  = 0;
    size_t valueLength = 0;

    std::unique_ptr<uint8_t, decltype(qfree)*> name(qmalloc(256u), qfree);
    std::unique_ptr<uint8_t, decltype(qfree)*> value(qalloc(outputCapacity), qfree);

    while(inputIndex < inputSize) {
        inputIndex += BDP::readPair(Global::BDP832, (uint8_t*)input, (uint8_t*)name.get(), (uint64_t*)&nameLength, (uint8_t*)value.get(), (uint64_t*)&valueLength);

        uint8_t nameByte = *name;
        
        // Only compares the first byte, for performance reasons.
        if(nameByte == *OSH_PLAINTEXT_MARKER) {
            while(outputSize + valueLength > outputCapacity)
                qexpand(output.get(), outputCapacity);
            memcpy(output.get() + outputSize, value.get(), valueLength);
        } else if(nameByte == *OSH_TEMPLATE_MARKER) {

        }
    }

    return BinaryData(nullptr, 0);
}
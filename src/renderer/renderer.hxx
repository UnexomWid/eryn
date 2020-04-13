#ifndef ERYN_RENDERER_HXX_GUARD
#define ERYN_RENDERER_HXX_GUARD

#include "bridge.hxx"
#include "../def/warnings.dxx"
#include "../../lib/buffer.hxx"

void renderFile(BridgeData data, const char* path, const char* outputPath);

BinaryData renderBytes(BridgeData data, uint8_t* input, size_t inputSize);
BinaryData renderBytes(BridgeData data, uint8_t* input, size_t inputSize, uint8_t* content, size_t contentSize, uint8_t* parentContent, size_t parentContentSize);

void renderBytes(BridgeData data, uint8_t* input, size_t inputSize, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity, uint8_t* content, size_t contentSize, uint8_t* parentContent, size_t parentContentSize);

#endif
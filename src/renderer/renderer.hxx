#ifndef ERYN_RENDERER_HXX_GUARD
#define ERYN_RENDERER_HXX_GUARD

#include "bridge.hxx"
#include "../def/warnings.dxx"
#include "../../lib/buffer.hxx"

#include <unordered_set>

BinaryData render(BridgeData data, const char* path);

BinaryData renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, std::unordered_set<std::string>* recompiled);
BinaryData renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, const uint8_t* content, size_t contentSize, const uint8_t* parentContent, size_t parentContentSize, std::unordered_set<std::string>* recompiled);
void renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity, const uint8_t* content, size_t contentSize, const uint8_t* parentContent, size_t parentContentSize, std::unordered_set<std::string>* recompiled);

#endif
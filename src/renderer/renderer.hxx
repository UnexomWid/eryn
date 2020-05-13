#ifndef ERYN_RENDERER_HXX_GUARD
#define ERYN_RENDERER_HXX_GUARD

#include "bridge.hxx"
#include "../def/warnings.dxx"
#include "../../lib/buffer.hxx"

#include <unordered_set>

BinaryData render(BridgeData data, const char* path);
BinaryData renderString(BridgeData data, const char* alias);

BinaryData renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, std::unordered_set<std::string>* recompiled, bool isString);
BinaryData renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, const uint8_t* content, size_t contentSize, std::unordered_set<std::string>* recompiled, bool isString);
void renderBytes(BridgeData data, const uint8_t* input, size_t inputSize, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity, const uint8_t* content, size_t contentSize, std::unordered_set<std::string>* recompiled, bool isString);

#endif
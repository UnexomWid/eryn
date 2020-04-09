#ifndef ERYN_RENDERER_HXX_GUARD
#define ERYN_RENDERER_HXX_GUARD

#include "bridge.hxx"
#include "../../lib/buffer.hxx"

BinaryData renderBytes(BridgeData data, uint8_t* inputBuffer, size_t inputSize);
void renderFile(BridgeData data, const char* path, const char* outputPath);

#endif
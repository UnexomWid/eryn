#ifndef ERYN_RENDERER_HXX_GUARD
#define ERYN_RENDERER_HXX_GUARD

#include "../../lib/buffer.hxx"

BinaryData renderBytes(uint8_t* inputBuffer, size_t inputSize);
void renderFile(const char* path, const char* outputPath);

#endif
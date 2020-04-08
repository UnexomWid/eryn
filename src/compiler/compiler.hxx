#ifndef ERYN_COMPILER_HXX_GUARD
#define ERYN_COMPILER_HXX_GUARD

#include "../../lib/buffer.hxx"

#include <cstddef>
#include <cstdint>

#define COMPILER_ERROR_CHUNK_SIZE 20u

BinaryData compileBytes(uint8_t* inputBuffer, size_t inputSize, const char* wd);
void compileFile(const char* wd, const char* path, const char* outputPath);

uint8_t* componentPathToAbsolute(const char* wd, const char* componentPath, size_t componentPathLength, size_t &absoluteLength);

#endif
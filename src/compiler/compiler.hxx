#ifndef ERYN_COMPILER_HXX_GUARD
#define ERYN_COMPILER_HXX_GUARD

#include "../def/warnings.dxx"
#include "../../lib/buffer.hxx"

#include <cstddef>
#include <cstdint>

#define COMPILER_ERROR_CHUNK_SIZE 20u

void compile(const char* path);

BinaryData compileFile(const char* path);

BinaryData compileBytes(uint8_t* inputBuffer, size_t inputSize, const char* wd);

#endif
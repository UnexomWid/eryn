#ifndef ERYN_COMPILER_HXX_GUARD
#define ERYN_COMPILER_HXX_GUARD

#include "filter.hxx"
#include "../def/warnings.dxx"
#include "../../lib/buffer.hxx"

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

#define COMPILER_PATH_SEPARATOR '/'
#define COMPILER_PATH_MAX_LENGTH 4096
#define COMPILER_ERROR_CHUNK_SIZE 20u

void compile(const char* path);
void compileString(const char* alias, const char* str);
void compileDir(const char* path, std::vector<std::string> filters);
void compileDir(const char* path, const char* rel, const FilterInfo& info);

BinaryData compileFile(const char* path);
BinaryData compileBytes(const uint8_t* inputBuffer, size_t inputSize, const char* wd, const char* path = "");

#endif
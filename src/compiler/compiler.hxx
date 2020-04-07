#ifndef ERYN_COMPILER_HXX_GUARD
#define ERYN_COMPILER_HXX_GUARD

#include "../def/osh.dxx"

#include <cstddef>
#include <cstdint>

#define LOG_DEBUG(...) 
#define LOG_INFO(...)  printf("[eryn] "##__VA_ARGS__);
#define LOG_ERROR(...) fprintf(stderr, "[eryn] "##__VA_ARGS__);

#define COMPILER_ERROR_CHUNK_SIZE 20u

OSHData compileBytes(uint8_t* inputBuffer, size_t inputSize, const char* wd);
void compileFile(const char* wd, const char* path, const char* outputPath);

uint8_t* componentPathToAbsolute(const char* wd, const char* componentPath, size_t componentPathLength, size_t &absoluteLength);

#endif
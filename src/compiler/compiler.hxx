#ifndef ERYN_COMPILER_HXX_GUARD
#define ERYN_COMPILER_HXX_GUARD

#include "../def/osh.dxx"

#include <cstddef>
#include <cstdint>

#define LOG_DEBUG(...) printf("[eryn] "##__VA_ARGS__);
#define LOG_INFO(...)  printf("[eryn] "##__VA_ARGS__);
#define LOG_ERROR(...) fprintf(stderr, "[eryn] "##__VA_ARGS__);

#define COMPILER_ERROR_CHUNK_SIZE 20u

OSHData compileBytes(uint8_t* inputBuffer, size_t inputSize);
void compileFile(const char* path, const char* outputPath);

#endif
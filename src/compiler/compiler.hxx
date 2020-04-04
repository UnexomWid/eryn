#ifndef ERYN_COMPILER_HXX_GUARD
#define ERYN_COMPILER_HXX_GUARD

#include <cstddef>

#define LOG_DEBUG(...) printf("[eryn] "##__VA_ARGS__);
#define LOG_INFO(...)  printf("[eryn] "##__VA_ARGS__);
#define LOG_ERROR(...) fprintf(stderr, "[eryn] "##__VA_ARGS__);

#define COMPILER_ERROR_CHUNK_SIZE 20u

void compileFile(const char* path, const char* outputPath);

#endif
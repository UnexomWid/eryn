#ifndef ERYN_COMPILER_HXX_GUARD
#define ERYN_COMPILER_HXX_GUARD

#include <cstddef>

#define LOG_DEBUG(...) printf(##__VA_ARGS__);
#define LOG_INFO(...)  printf(##__VA_ARGS__);
#define LOG_ERROR(...) fprintf(stderr, ##__VA_ARGS__);

#define COMPILER_PLAINTEXT_MARKER                         reinterpret_cast<uint8_t*>("p")
#define COMPILER_TEMPLATE_MARKER                          reinterpret_cast<uint8_t*>("t")
#define COMPILER_TEMPLATE_CONDITIONAL_START_MARKER        reinterpret_cast<uint8_t*>("?")
#define COMPILER_TEMPLATE_CONDITIONAL_END_MARKER          reinterpret_cast<uint8_t*>("!")
#define COMPILER_TEMPLATE_LOOP_START_MARKER               reinterpret_cast<uint8_t*>("@")
#define COMPILER_TEMPLATE_LOOP_END_MARKER                 reinterpret_cast<uint8_t*>("#")

#define COMPILER_PLAINTEXT_MARKER_LENGTH                  1u
#define COMPILER_TEMPLATE_MARKER_LENGTH                   1u
#define COMPILER_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH 1u
#define COMPILER_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH   1u
#define COMPILER_TEMPLATE_LOOP_START_MARKER_LENGTH        1u
#define COMPILER_TEMPLATE_LOOP_END_MARKER_LENGTH          1u

#define COMPILER_ERROR_CHUNK_SIZE 15

void compileFile(const char* path, const char* outputPath);

#endif
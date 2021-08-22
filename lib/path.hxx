#ifndef ERYN_COMMON_PATH_HXX_GUARD
#define ERYN_COMMON_PATH_HXX_GUARD

#include <cstddef>

bool pathIsAbsolute(const char* path, size_t length);
bool pathIsRelative(const char* path, size_t length);
size_t pathDirEndIndex(const char* path, size_t length);

#endif
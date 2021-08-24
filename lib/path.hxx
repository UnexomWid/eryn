#ifndef ERYN_COMMON_PATH_HXX_GUARD
#define ERYN_COMMON_PATH_HXX_GUARD

#include <cstddef>

namespace path {
bool   is_absolute(const char* path, size_t length);
bool   is_relative(const char* path, size_t length);
size_t dir_end_index(const char* path, size_t length);
}

#endif
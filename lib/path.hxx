#ifndef ERYN_COMMON_PATH_HXX_GUARD
#define ERYN_COMMON_PATH_HXX_GUARD

#include <cstddef>
#include <string>

namespace path {
bool        is_absolute(const char* path, size_t length);
bool        is_relative(const char* path, size_t length);
size_t      dir_end_index(const char* path, size_t length);
std::string append_or_absolute(const char* wd, const char* path, size_t length);
std::string append_or_absolute(const char* wd, const std::string& path);
std::string append_or_absolute(const std::string& wd, const std::string& path);
void        normalize(std::string& path);
}

#endif
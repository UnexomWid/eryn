#include "path.hxx"
#include "str.hxx"
#include "../src/def/os.dxx"

bool path::is_absolute(const char* path, size_t length) {
    #ifdef OS_WINDOWS
        return (length > 2)
            && (str::to_upper(static_cast<unsigned char>(path[0])) >= 'A')
            && (str::to_upper(static_cast<unsigned char>(path[0])) <= 'Z')
            && (path[1] == ':')
            && (path[2] == '/' || path[2] == '\\');
    #else
        return (length > 1)
            && (path[0] == '/');
    #endif
}

bool path::is_relative(const char* path, size_t length) {
    return !path::is_absolute(path, length);
}

size_t path::dir_end_index(const char* path, size_t length) {
    size_t index    = 0;
    size_t endIndex = 0;

    while(index < length) {
        if(*path == '/' || *path == '\\') {
            endIndex = index;
        }
        ++index;
        ++path;
    }

    return endIndex;
}
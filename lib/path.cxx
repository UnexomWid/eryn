#include "path.hxx"
#include "str.hxx"
#include "../src/def/os.dxx"

#include <cstring>

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

std::string path::append_or_absolute(const char* wd, const char* path, size_t length) {
    std::string str(reinterpret_cast<const char*>(path), length);

    if(strlen(wd) == 0) {
        return str;
    }

    if(path::is_absolute(str.c_str(), str.size())) {
        return str;
    }

    std::string pathBuilder(wd);

    if(pathBuilder[pathBuilder.size() - 1] != '/' && pathBuilder[pathBuilder.size() - 1] != '\\') {
        pathBuilder += '/';
    }

    pathBuilder.append(str);

    return pathBuilder;
}

std::string path::append_or_absolute(const char* wd, const std::string& path) {
    if(strlen(wd) == 0) {
        return path;
    }

    if(path::is_absolute(path.c_str(), path.size())) {
        return path;
    }

    std::string pathBuilder(wd);

    if(pathBuilder[pathBuilder.size() - 1] != '/' && pathBuilder[pathBuilder.size() - 1] != '\\') {
        pathBuilder += '/';
    }

    pathBuilder.append(path);

    return pathBuilder;
}

std::string path::append_or_absolute(const std::string& wd, const std::string& path) {
    if(wd.size() == 0) {
        return path;
    }

    if(path::is_absolute(path.c_str(), path.size())) {
        return path;
    }

    std::string pathBuilder(wd);

    if(pathBuilder[pathBuilder.size() - 1] != '/' && pathBuilder[pathBuilder.size() - 1] != '\\') {
        pathBuilder += '/';
    }

    pathBuilder.append(path);

    return pathBuilder;
}

void path::normalize(std::string& path) {
    for(auto& c : path) {
        if(c == '\\') {
            c = '/';
        }
    }

    auto index = path.size() - 1;

    while(index > 0 && path[index] == '/') {
        --index;
    }

    path = path.substr(0, index + 1);
}
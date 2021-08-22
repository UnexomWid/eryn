#ifndef FILTER_HXX_GUARD
#define FILTER_HXX_GUARD

#include <vector>
#include <cstddef>

struct FilterInfo {
    struct Filter {
        char* glob;

        std::vector<char*> exclusions;

        Filter(const char* pattern);
        Filter(const char* pattern, size_t count);
        ~Filter();
    };

    std::vector<Filter*> filters;
    std::vector<char*> exclusions;

    ~FilterInfo();

    void addFilter(const char* pattern);
    void addFilter(const char* pattern, size_t count);

    void addExclusion(const char* pattern);
    void addExclusion(const char* pattern, size_t count);

    bool isFileFiltered(const char* path) const;
    bool isDirFiltered(const char* path) const;
};

#endif
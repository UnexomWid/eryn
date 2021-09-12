#ifndef FILTER_HXX_GUARD
#define FILTER_HXX_GUARD

#include <vector>
#include <cstddef>

struct FilterInfo {
    struct Filter {
        std::string glob;

        std::vector<std::string> exclusions;

        Filter(std::string pattern);
        Filter(std::string pattern, size_t count);
    };

    std::vector<Filter> filters;
    std::vector<std::string> exclusions;

    void add_filter(const std::string& pattern);
    void add_filter(const std::string& pattern, size_t count);

    void add_exclusion(const std::string& pattern);
    void add_exclusion(const std::string& pattern, size_t count);

    bool is_file_filtered(const std::string& path) const;
    bool is_dir_filtered(const std::string& path) const;
};

#endif
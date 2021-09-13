#include "filter.hxx"

#include <cstdlib>
#include <stdexcept>

#include "str.hxx"

#include "../../lib/globlib.h"
#include "../../lib/buffer.hxx"
#include "../../lib/remem.hxx"

using Filter = FilterInfo::Filter;

Filter::Filter(std::string pattern) : glob(pattern), exclusions() { }
Filter::Filter(std::string pattern, size_t count) : glob(pattern, 0, count), exclusions() { }

void FilterInfo::add_filter(const std::string& pattern) {
    filters.emplace_back(pattern);
}

void FilterInfo::add_filter(const std::string&  pattern, size_t count) {
    filters.emplace_back(pattern, count);
}

void FilterInfo::add_exclusion(const std::string&  pattern) {
    auto exclusion = pattern;
    bool found = false;

    for(size_t i = 0; i < filters.size();) {
        auto& filter = filters[i];

        if(match(exclusion.c_str(), filter.glob.c_str(), GLOB_MATCH_DOTFILES)) {
            if(filter.glob == exclusion) {
                filters.erase(filters.begin() + i);
                continue;
            } else {
                filter.exclusions.push_back(exclusion);
                found = true;
            }
        } else if(match(filter.glob.c_str(), exclusion.c_str(), GLOB_MATCH_DOTFILES)) {
            filters.erase(filters.begin() + i);
            continue;
        }
        ++i;
    }

    if(!found) {
        exclusions.push_back(exclusion);
    }
}

void FilterInfo::add_exclusion(const std::string& pattern, size_t count) {
    std::string exclusion(pattern, 0, count);
    bool found = false;

    for(size_t i = 0; i < filters.size();) {
        auto& filter = filters[i];

        if(match(exclusion.c_str(), filter.glob.c_str(), GLOB_MATCH_DOTFILES)) {
            if(filter.glob == exclusion) {
                filters.erase(filters.begin() + i);
                continue;
            } else {
                filter.exclusions.push_back(exclusion);
                found = true;
            }
        } else if(match(filter.glob.c_str(), exclusion.c_str(), GLOB_MATCH_DOTFILES)) {;
            filters.erase(filters.begin() + i);
            continue;
        }
        ++i;
    }

    if(!found) {
        exclusions.push_back(exclusion);
    }
}

bool FilterInfo::is_file_filtered(const std::string& path) const {
    bool filtered = false;

    for(auto& filter : filters) {
        if(match(path.c_str(), filter.glob.c_str(), GLOB_MATCH_DOTFILES)) {
            filtered = true;

            for(auto& exclusion : filter.exclusions) {
                if(match(path.c_str(), exclusion.c_str(), GLOB_MATCH_DOTFILES)) {
                    filtered = false;
                }
            }
        }
    }

    if(!filtered) {
        return false;
    }

    for(auto& exclusion : exclusions) {
        if(match(path.c_str(), exclusion.c_str(), GLOB_MATCH_DOTFILES)) {
            return false;
        }
    }

    return true;
}

bool FilterInfo::is_dir_filtered(const std::string& path) const {
    for(auto& filter : filters) {
        if(match(path.c_str(), filter.glob.c_str(), GLOB_MATCH_DOTFILES)) {
            return true;
        }
    }
    
    for(auto& exclusion : exclusions) {
        if(match(path.c_str(), exclusion.c_str(), GLOB_MATCH_DOTFILES)) {
            return false;
        }
    }

    return true;
}
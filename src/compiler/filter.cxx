#include "filter.hxx"

#include "../../lib/remem.hxx"
#include "../../lib/globlib.h"
#include "../../lib/buffer.hxx"

#include "../common/str.hxx"

#include <cstdlib>
#include <stdexcept>

using Filter = FilterInfo::Filter;

Filter::Filter(const char* pattern) : glob(strDup(pattern)), exclusions() { }
Filter::Filter(const char* pattern, size_t count) : glob(strDup(pattern, count)), exclusions() { }
Filter::~Filter() {
    re::free(glob);

    for(auto exclusion : exclusions)
        re::free(exclusion);
}

FilterInfo::~FilterInfo() {
    for(auto filter : filters)
        delete filter;
    for(auto exclusion : exclusions)
        re::free(exclusion);
}

void FilterInfo::addFilter(const char* const pattern) {
    filters.push_back(new Filter(pattern));
}

void FilterInfo::addFilter(const char* const pattern, size_t count) {
    filters.push_back(new Filter(pattern, count));
}

void FilterInfo::addExclusion(const char* const pattern) {
    char* exclusion = strDup(pattern);
    bool found = false;

    for(size_t i = 0; i < filters.size();) {
        Filter* filter = filters[i];

        if(match(exclusion, filter->glob, GLOB_MATCH_DOTFILES)) {
            if(strcmp(filter->glob, exclusion) == 0) {
                delete filters[i];
                filters.erase(filters.begin() + i);
                continue;
            } else {
                filter->exclusions.push_back(exclusion);
                found = true;
            }
        } else if(match(filter->glob, exclusion, GLOB_MATCH_DOTFILES)) {
            delete filters[i];
            filters.erase(filters.begin() + i);
            continue;
        }
        ++i;
    }

    if(!found)
        exclusions.push_back(exclusion);
}

void FilterInfo::addExclusion(const char* const pattern, size_t count) {
    char* exclusion = strDup(pattern, count);
    bool found = false;

    for(size_t i = 0; i < filters.size();) {
        Filter* filter = filters[i];

        if(match(exclusion, filter->glob, GLOB_MATCH_DOTFILES)) {
            if(strcmp(filter->glob, exclusion) == 0) {
                delete filters[i];
                filters.erase(filters.begin() + i);
                continue;
            } else {
                filter->exclusions.push_back(exclusion);
                found = true;
            }
        } else if(match(filter->glob, exclusion, GLOB_MATCH_DOTFILES)) {
            delete filters[i];
            filters.erase(filters.begin() + i);
            continue;
        }
        ++i;
    }

    if(!found)
        exclusions.push_back(exclusion);
}

bool FilterInfo::isFileFiltered(const char* const path) const {
    bool filtered = false;

    for(size_t i = 0; i < filters.size(); ++i) {
        Filter* filter = filters[i];
        if(match(path, filter->glob, GLOB_MATCH_DOTFILES)) {
            filtered = true;
            for(size_t j = 0; j < filter->exclusions.size(); ++j)
                if(match(path, filter->exclusions[j], GLOB_MATCH_DOTFILES))
                    filtered = false;
        }
    }

    if(!filtered)
        return false;

    for(size_t i = 0; i < exclusions.size(); ++i)
        if(match(path, exclusions[i], GLOB_MATCH_DOTFILES))
            return false;

    return true;
}

bool FilterInfo::isDirFiltered(const char* const path) const {
    for(size_t i = 0; i < filters.size(); ++i) {
        Filter* filter = filters[i];
        if(match(path, filter->glob, GLOB_MATCH_DOTFILES))
            return true;
    }
    
    for(size_t i = 0; i < exclusions.size(); ++i)
        if(match(path, exclusions[i], GLOB_MATCH_DOTFILES))
            return false;

    return true;
}
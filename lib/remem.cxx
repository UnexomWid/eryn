/**
 * Remem (https://github.com/UnexomWid/remem)
 *
 * This project is licensed under the MIT license.
 * Copyright (c) 2020 UnexomWid (https://uw.exom.dev)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "remem.hxx"

#include <cstdio>
#include <cstdlib>

#ifdef _MSC_VER
    #pragma warning(disable: 4293)
#endif

#if defined(REMEM_ENABLE_MAPPING)
    std::unordered_map<void*, re::AddressInfo> map;
    size_t totalSize = 0;
#endif

#if !defined(REMEM_EXPAND_FACTOR)
    #define REMEM_EXPAND_FACTOR 2
#endif

// Rounds the size to the nearest power of 2 that is >= size.
void adjustSize(size_t &size) {
    --size;
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;

    if constexpr(sizeof(size_t) == 8)
        size |= size >> 32;
    ++size;
}

void* allocateBlock(size_t size, const char* who = nullptr, const char* file = nullptr, size_t line = 0) {
    #if !defined(REMEM_DISABLE_ALIGNING)
        adjustSize(size);
    #endif

    void* ptr = ::malloc(size);

    if(!ptr)
        throw std::bad_alloc();

    #if defined(REMEM_ENABLE_MAPPING)
        map[ptr] = re::AddressInfo(who == nullptr ? "unknown" : who, size);
        totalSize += size;

        #if defined(REMEM_ENABLE_LOGGING)
            if(who == nullptr)
                printf("[memory] Allocated 'unknown' (%p) | %zu byte(s)\n", ptr, size);
            else if(file == nullptr)
                printf("[memory] Allocated '%s' (%p) | %zu byte(s)\n", who, ptr, size);
            else if(line == 0)
                printf("[memory] Allocated '%s' (%p) | %zu byte(s) in '%s'\n", who, ptr, size, file);
            else printf("[memory] Allocated '%s' (%p) | %zu byte(s) in '%s' at line %zu\n", who, ptr, size, file, line);
        #endif
    #endif

    return ptr;
}

void* operator new[](size_t size) {
    return allocateBlock(size);
}

void* operator new[](size_t size, const char* who, const char* file, size_t line) {
    return allocateBlock(size, who, file, line);
}

void operator delete[](void* ptr) noexcept {
    if (ptr) {
        ::free(ptr);

        #if defined(REMEM_ENABLE_MAPPING)
            if(map.find(ptr) != map.end()) {
                #if defined(REMEM_ENABLE_LOGGING)
                    printf("[memory] Freed '%s' (%p) | %zu byte(s)\n", map[ptr].who.c_str(), ptr, map[ptr].size);
                #endif

                totalSize -= map[ptr].size;
                map.erase(ptr);
            }
        #endif
    } else {
        #if defined(REMEM_ENABLE_MAPPING) && defined(REMEM_ENABLE_LOGGING)
            if(map.find(ptr) != map.end())
                printf("[memory] Attempted to free nullptr");
        #endif
    }
}

#if defined(REMEM_ENABLE_MAPPING)
    const std::unordered_map<void*, re::AddressInfo>& re::mem() noexcept {
        return map;
    }

    void re::memPrint() noexcept {
        if(map.size() != 0) {
            printf("\n[memory] Map: %zu byte(s)\n", totalSize);
            for (auto entry : mem())
                printf("%p -> %s: %zu byte(s)\n", entry.first, entry.second.who.c_str(), entry.second.size);
            printf("\n");
        } else {
            printf("\n[memory] Map: empty\n");
        }
    }

    size_t re::memSize() noexcept {
        return totalSize;
    }
#endif

void* re::malloc(size_t size, const char* who, const char* file, size_t line) {
    #if !defined(REMEM_DISABLE_MALLOC_ALIGNING) && !defined(REMEM_DISABLE_ALIGNING)
        adjustSize(size);
    #endif

        void* ptr = ::malloc(size);

        if(!ptr)
            throw std::bad_alloc();

    #if defined(REMEM_ENABLE_MAPPING)
        map[ptr] = re::AddressInfo(who == nullptr ? "unknown" : who, size);
        totalSize += size;

        #if defined(REMEM_ENABLE_LOGGING)
            if(who == nullptr)
                printf("[memory] Allocated 'unknown' (%p) | %zu byte(s)\n", ptr, size);
            else if(file == nullptr)
                printf("[memory] Allocated '%s' (%p) | %zu byte(s)\n", who, ptr, size);
            else if(line == 0)
                printf("[memory] Allocated '%s' (%p) | %zu byte(s) in '%s'\n", who, ptr, size, file);
            else printf("[memory] Allocated '%s' (%p) | %zu byte(s) in '%s' at line %zu\n", who, ptr, size, file, line);
        #endif
    #endif

    return ptr;
}

void* re::alloc(size_t&  size, const char* who, const char* file, size_t line) {
    #if !defined(REMEM_DISABLE_ALIGNING)
        adjustSize(size);
    #endif

    void* ptr = ::malloc(size);

    if(!ptr)
        throw std::bad_alloc();

    #if defined(REMEM_ENABLE_MAPPING)
        map[ptr] = re::AddressInfo(who == nullptr ? "unknown" : who, size);
        totalSize += size;

        #if defined(REMEM_ENABLE_LOGGING)
            if(who == nullptr)
                printf("[memory] Allocated 'unknown' (%p) | %zu byte(s)\n", ptr, size);
            else if(file == nullptr)
                printf("[memory] Allocated '%s' (%p) | %zu byte(s)\n", who, ptr, size);
            else if(line == 0)
                printf("[memory] Allocated '%s' (%p) | %zu byte(s) in '%s'\n", who, ptr, size, file);
            else printf("[memory] Allocated '%s' (%p) | %zu byte(s) in '%s' at line %zu\n", who, ptr, size, file, line);
        #endif
    #endif

    return ptr;
}

void* re::realloc(void* ptr, size_t size, const char* file, size_t line) {
    #if !defined(REMEM_DISABLE_REALLOC_ALIGNING) && !defined(REMEM_DISABLE_ALIGNING)
        adjustSize(size);
    #endif

    void* newPtr = ::realloc(ptr, size);

    if(!newPtr)
        throw std::bad_alloc();

    #if defined(REMEM_ENABLE_MAPPING)
        if(map.find(ptr) != map.end()) {
            #if defined(REMEM_ENABLE_LOGGING)
                if(file == nullptr)
                    printf("[memory] Reallocated '%s' (%p) | %zu -> %zu byte(s)\n", map[ptr].who.c_str(), newPtr, map[ptr].size, size);
                else if(line == 0)
                    printf("[memory] Reallocated '%s' (%p) | %zu -> %zu byte(s) in '%s'\n", map[ptr].who.c_str(), newPtr, map[ptr].size, size, file);
                else printf("[memory] Reallocated '%s' (%p) | %zu -> %zu byte(s) in '%s' at line %zu\n", map[ptr].who.c_str(), newPtr, map[ptr].size, size, file, line);
            #endif

            totalSize -= map[ptr].size;
            totalSize += size;

            map[ptr].size = size;

            if(ptr != newPtr) {
                map[newPtr] = map[ptr];
                map.erase(ptr);
            }
        }
    #endif

    return newPtr;
}

void* re::expand(void* ptr, size_t& size, const char* file, size_t line) {
    size *= REMEM_EXPAND_FACTOR;

    #if !defined(REMEM_DISABLE_ALIGNING)
        adjustSize(size);
    #endif

    void* newPtr = ::realloc(ptr, size);

    if(!newPtr)
        throw std::bad_alloc();

    #if defined(REMEM_ENABLE_MAPPING)
        if(map.find(ptr) != map.end()) {
            #if defined(REMEM_ENABLE_LOGGING)
                if(file == nullptr)
                    printf("[memory] Expanded '%s' (%p) | %zu -> %zu byte(s)\n", map[ptr].who.c_str(), newPtr, map[ptr].size, size);
                else if(line == 0)
                    printf("[memory] Expanded '%s' (%p) | %zu -> %zu byte(s) in '%s'\n", map[ptr].who.c_str(), newPtr, map[ptr].size, size, file);
                else printf("[memory] Expanded '%s' (%p) | %zu -> %zu byte(s) in '%s' at line %zu\n", map[ptr].who.c_str(), newPtr, map[ptr].size, size, file, line);
            #endif

            totalSize -= map[ptr].size;
            totalSize += size;

            map[ptr].size = size;

            if(ptr != newPtr) {
                map[newPtr] = map[ptr];
                map.erase(ptr);
            }
        }
    #endif

    return newPtr;
}

void re::free(void* ptr) noexcept {
    if (ptr) {
        ::free(ptr);

        #if defined(REMEM_ENABLE_MAPPING)
            if(map.find(ptr) != map.end()) {
                #if defined(REMEM_ENABLE_LOGGING)
                    printf("[memory] Freed '%s' (%p) | %zu byte(s)\n", map[ptr].who.c_str(), ptr, map[ptr].size);
                #endif

                totalSize -= map[ptr].size;
                map.erase(ptr);
            }
        #endif
    } else {
        #if defined(REMEM_ENABLE_MAPPING) && defined(REMEM_ENABLE_LOGGING)
            if(map.find(ptr) != map.end())
                printf("[memory] Attempted to free nullptr");
        #endif
    }
}
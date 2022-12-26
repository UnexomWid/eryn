/**
 * Remem (https://github.com/UnexomWid/remem)
 *
 * This project is licensed under the MIT license.
 * Copyright (c) 2020-2021 UnexomWid (https://uw.exom.dev)
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
#ifndef REMEM_HXX_GUARD
#define REMEM_HXX_GUARD

#include <new>
#include <string>
#include <cstddef>
#include <unordered_map>

#if defined(REMEM_ENABLE_MAPPING)
    #define new(who) new(who, __FILE__, __LINE__)
#else
    #define new(who) new
#endif

#define REMEM_ALLOC(size, who) re::alloc(size, who, __FILE__, __LINE__)
#define REMEM_MALLOC(size, who) re::malloc(size, who, __FILE__, __LINE__)
#define REMEM_REALLOC(ptr, size) re::realloc(ptr, size, __FILE__, __LINE__)
#define REMEM_EXPAND(ptr, size) re::expand(ptr, size, __FILE__, __LINE__)
#define REMEM_FREE(ptr) re::free(ptr)

void* operator new[](size_t size);
void* operator new[](size_t size, const char* who, const char* file, size_t line);
void  operator delete[](void* ptr) noexcept;

namespace re {
    struct AddressInfo {
        std::string who;
        size_t size;

        AddressInfo() { }
        AddressInfo(const char* w, size_t sz) : who(w), size(sz) { }
    };

    #if defined(REMEM_ENABLE_MAPPING)
        const std::unordered_map<void*, AddressInfo>& mem() noexcept;

        void   mem_print() noexcept;
        size_t mem_size()  noexcept;
    #endif

    void* malloc(size_t size, const char* who = nullptr, const char* file = nullptr, size_t line = 0);
    void* alloc(size_t& size, const char* who = nullptr, const char* file = nullptr, size_t line = 0);

    void* realloc(void* ptr, size_t size, const char* file = nullptr, size_t line = 0);
    void* expand(void* ptr, size_t& size, const char* file = nullptr, size_t line = 0);

    void  free(void* ptr) noexcept;
}

#endif
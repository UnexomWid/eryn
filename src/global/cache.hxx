#ifndef ERYN_GLOBAL_CACHE_HXX_GUARD
#define ERYN_GLOBAL_CACHE_HXX_GUARD

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#include "../../lib/buffer.hxx"

namespace Global {
    class Cache {
        private:
            static std::vector<BinaryData> data;
            static std::unordered_map<std::string, size_t> entries;

        public:
            static void       addEntry(std::string key, BinaryData value);
            static BinaryData getEntry(std::string key);
            static size_t     getRawEntry(std::string key);
            static bool       hasEntry(std::string key);

            static void       addEntry(const char* key, BinaryData value);
            static BinaryData getEntry(const char* key);
            static size_t     getRawEntry(const char* key);
            static bool       hasEntry(const char* key);

            static const BinaryData& getData(size_t index);
            static       void        setData(size_t index, const BinaryData& data);

            static void destroy();
    };
}

#endif
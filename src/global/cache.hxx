#ifndef ERYN_GLOBAL_CACHE_HXX_GUARD
#define ERYN_GLOBAL_CACHE_HXX_GUARD

#include "../../lib/buffer.hxx"

#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Global {
    class Cache {
        private:
            static std::vector<BinaryData> data;
            static std::unordered_map<std::string, size_t> entries;

        public:
            static void addEntry(std::string key, BinaryData value);
            static BinaryData getEntry(std::string key);
            static bool hasEntry(std::string key);

            static void addEntry(const char* key, BinaryData value);
            static BinaryData getEntry(const char* key);
            static bool hasEntry(const char* key);

            static void destroy();
    };
}

#endif
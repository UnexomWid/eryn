#ifndef ERYN_GLOBAL_CACHE_HXX_GUARD
#define ERYN_GLOBAL_CACHE_HXX_GUARD

#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Global {
    class Cache {
        private:
            static std::vector<const uint8_t*> data;
            static std::unordered_map<std::string, size_t> entries;

        public:
            static void addEntry(std::string key, const uint8_t* value);
            static const uint8_t* getEntry(std::string key);
            static bool hasEntry(std::string key);

            static void addEntry(const char* key, const uint8_t* value);
            static const uint8_t* getEntry(const char* key);
            static bool hasEntry(const char* key);

            static void destroy();
    };
}

#endif
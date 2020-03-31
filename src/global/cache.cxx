#include "cache.hxx"

#include <vector>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>

std::vector<const uint8_t*> Global::Cache::data;
std::unordered_map<std::string, size_t> Global::Cache::entries;

void Global::Cache::addEntry(std::string key, const uint8_t* value) {
    auto index = std::find(Global::Cache::data.begin(), Global::Cache::data.end(), value);

    if(index == Global::Cache::data.end()) {
        index = Global::Cache::data.begin() + Global::Cache::data.size();
        Global::Cache::data.push_back(value);
    }

    Global::Cache::entries[key] = std::distance(Global::Cache::data.begin(), index);
}

const uint8_t* Global::Cache::getEntry(std::string key) {
    if(Global::Cache::hasEntry(key))
        return Global::Cache::data[Global::Cache::entries[key]];
    return nullptr;
}

bool Global::Cache::hasEntry(std::string key) {
    return Global::Cache::entries.find(key) != Global::Cache::entries.end();
}

void Global::Cache::addEntry(const char* key, const uint8_t* value) {
    Global::Cache::addEntry(std::string(key), value);
}

const uint8_t* Global::Cache::getEntry(const char* key) {
    return Global::Cache::getEntry(std::string(key));
}

bool Global::Cache::hasEntry(const char* key) {
    return Global::Cache::hasEntry(std::string(key));
}

void Global::Cache::destroy() {
    for(const uint8_t* entry : Global::Cache::data)
        free((uint8_t*) entry);
}
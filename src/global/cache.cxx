#include "cache.hxx"

#include <vector>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>

std::vector<BinaryData> Global::Cache::data;
std::unordered_map<std::string, size_t> Global::Cache::entries;

void Global::Cache::addEntry(std::string key, BinaryData value) {
    auto index = Global::Cache::data.begin();

    for(; index != Global::Cache::data.end(); ++index)
        if(index->data == value.data)
            break;

    if(index == Global::Cache::data.end()) {
        Global::Cache::data.push_back(value);
        index = Global::Cache::data.begin() + Global::Cache::data.size() - 1;
    }

    Global::Cache::entries[key] = std::distance(Global::Cache::data.begin(), index);
}

BinaryData Global::Cache::getEntry(std::string key) {
    if(Global::Cache::hasEntry(key))
        return Global::Cache::data[Global::Cache::entries[key]];
    return BinaryData();
}

size_t Global::Cache::getRawEntry(std::string key) {
    if(Global::Cache::hasEntry(key))
        return Global::Cache::entries[key];
    return -1;
}

bool Global::Cache::hasEntry(std::string key) {
    return Global::Cache::entries.find(key) != Global::Cache::entries.end();
}

void Global::Cache::addEntry(const char* key, BinaryData value) {
    Global::Cache::addEntry(std::string(key), value);
}

BinaryData Global::Cache::getEntry(const char* key) {
    return Global::Cache::getEntry(std::string(key));
}

size_t Global::Cache::getRawEntry(const char* key) {
    return Global::Cache::getRawEntry(std::string(key));
}

bool Global::Cache::hasEntry(const char* key) {
    return Global::Cache::hasEntry(std::string(key));
}

const BinaryData& Global::Cache::getData(size_t index) {
    return data[index];
}

void Global::Cache::setData(size_t index, const BinaryData &value) {
    data[index] = value;
}

void Global::Cache::destroy() {
    for(BinaryData &entry : Global::Cache::data)
        qfree((uint8_t*) entry.data);
}
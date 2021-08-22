#include "engine.hxx"

void Eryn::Cache::add(string key, Buffer&& value) {
    // TODO: test if the move constructor is called.
    entries[key] = value;
}

Buffer& Eryn::Cache::get(string key) {
    if(!has(key)) {
        throw ERYN_INTERNAL_EXCEPTION(("Cache item '" + key) + "' not found; get() must be guarded by has()");
    }

    return entries[key];
}

bool Eryn::Cache::has(string key) const {
    return entries.find(key) != entries.end();
}
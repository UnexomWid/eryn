#include "engine.hxx"

Eryn::Cache::~Cache() {
    for(auto& entry : entries) {
        ConstBuffer::finalize(entry.second);
    }
}

void Eryn::Cache::add(const string& key, ConstBuffer&& value) {
    if(has(key)) {
        ConstBuffer::finalize(get(key));
    }

    // TODO: test if the move constructor is called.
    entries[key] = value;
}

ConstBuffer& Eryn::Cache::get(const string& key) {
    if(!has(key)) {
        throw ERYN_INTERNAL_EXCEPTION(("Cache item '" + key) + "' not found; get() must be guarded by has()");
    }

    return entries[key];
}

bool Eryn::Cache::has(const string& key) const {
    return entries.find(key) != entries.end();
}
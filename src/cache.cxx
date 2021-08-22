class Cache {
    std::unordered_map<std::string, Buffer> entries;

  public:
    void    add(std::string key, Buffer value);
    Buffer& get(std::string key);
    bool    has(std::string key);
};

#include "engine.hxx"

void Eryn::Cache::add(string key, Buffer value) {

}

Buffer& Eryn::Cache::get(string key) {
    if(!has(key)) {
        
    }
}

bool Eryn::Cache::has(string key) {

}
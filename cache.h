#ifndef CACHE_H
#define CACHE_H

#include "common.h"
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <map>
#include <string>

class MessageCache {
private:
    std::vector<CacheEntry> cache;
    int capacity;
    int head;
    int tail;
    int size;
    std::map<std::string, int> index_map;
    mutable std::shared_mutex cache_mutex;
    
    uint64_t hits;
    uint64_t misses;
    
    int find_lru_index();
    std::string generate_message_id(const std::string& sender, time_t timestamp);

public:
    MessageCache(int capacity = CACHE_SIZE);
    ~MessageCache();
    
    bool insert(const std::string& sender, const std::string& content, time_t timestamp);
    bool lookup(const std::string& message_id, std::string& content);
    void update_access(const std::string& message_id);
    
    uint64_t get_hits() const;
    uint64_t get_misses() const;
    double get_hit_rate() const;
    int get_size() const;
};

#endif
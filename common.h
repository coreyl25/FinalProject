#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <cstdint>
#include <ctime>

// Server configuration
#define SERVER_PORT 8080
#define MAX_CLIENTS 50
#define THREAD_POOL_SIZE 6
#define BUFFER_SIZE 4096
#define CACHE_SIZE 50
#define TIME_QUANTUM_MS 100

// Message types
#define MSG_TEXT 0x01
#define MSG_JOIN 0x02
#define MSG_LEAVE 0x03
#define MSG_AUDIO 0x04
#define MSG_VIDEO 0x05
#define MSG_STATUS 0x06

// Message structure
struct Message {
    uint8_t type;
    uint32_t user_id;
    uint32_t payload_size;
    char sender[64];
    char payload[BUFFER_SIZE];
    time_t timestamp;
    
    Message() : type(0), user_id(0), payload_size(0), timestamp(0) {
        sender[0] = '\0';
        payload[0] = '\0';
    }
};

// Cache entry structure
struct CacheEntry {
    std::string message_id;
    std::string content;
    std::string sender;
    time_t timestamp;
    time_t last_access;
    int access_count;
    bool valid;
    
    CacheEntry() : timestamp(0), last_access(0), access_count(0), valid(false) {}
};

// Client information
struct ClientInfo {
    int socket_fd;
    std::string user_id;
    time_t connect_time;
    time_t last_active;
    bool active;
    
    ClientInfo() : socket_fd(-1), connect_time(0), last_active(0), active(false) {}
};

// Performance metrics
struct PerformanceMetrics {
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t page_faults_minor;
    uint64_t page_faults_major;
    int active_threads;
    int active_clients;
    
    PerformanceMetrics() : messages_sent(0), messages_received(0), 
                           cache_hits(0), cache_misses(0),
                           page_faults_minor(0), page_faults_major(0),
                           active_threads(0), active_clients(0) {}
};

#endif
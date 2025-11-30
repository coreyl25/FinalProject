#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"
#include <string>
#include <mutex>

struct ScheduledClient {
    int socket_fd;
    std::string user_id;
    time_t last_scheduled;
    ScheduledClient* next;
    
    ScheduledClient(int fd, const std::string& id) 
        : socket_fd(fd), user_id(id), last_scheduled(0), next(nullptr) {}
};

class RoundRobinScheduler {
private:
    ScheduledClient* head;
    ScheduledClient* current;
    int client_count;
    std::mutex scheduler_mutex;
    int time_quantum_ms;

public:
    RoundRobinScheduler(int quantum_ms = TIME_QUANTUM_MS);
    ~RoundRobinScheduler();
    
    void add_client(int socket_fd, const std::string& user_id);
    void remove_client(int socket_fd);
    ScheduledClient* get_next_client();
    int get_client_count() const;
    void print_schedule();
};

#endif
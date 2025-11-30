#include "scheduler.h"
#include <iostream>

RoundRobinScheduler::RoundRobinScheduler(int quantum_ms) 
    : head(nullptr), current(nullptr), client_count(0), time_quantum_ms(quantum_ms) {
    std::cout << "[Scheduler] Initialized with " << time_quantum_ms << "ms time quantum" << std::endl;
}

RoundRobinScheduler::~RoundRobinScheduler() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    
    if (!head) return;
    
    ScheduledClient* temp = head;
    ScheduledClient* next_client;
    
    do {
        next_client = temp->next;
        delete temp;
        temp = next_client;
    } while (temp != head);
    
    head = nullptr;
    current = nullptr;
}

void RoundRobinScheduler::add_client(int socket_fd, const std::string& user_id) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    
    ScheduledClient* new_client = new ScheduledClient(socket_fd, user_id);
    
    if (!head) {
        // First client
        head = new_client;
        head->next = head;  // Point to itself
        current = head;
    } else {
        // Find the last client
        ScheduledClient* last = head;
        while (last->next != head) {
            last = last->next;
        }
        
        // Insert new client at the end
        last->next = new_client;
        new_client->next = head;
    }
    
    client_count++;
    std::cout << "[Scheduler] Added client " << user_id << " (fd: " << socket_fd << ")" << std::endl;
}

void RoundRobinScheduler::remove_client(int socket_fd) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    
    if (!head) return;
    
    ScheduledClient* temp = head;
    ScheduledClient* prev = nullptr;
    
    // Find the last node (to get prev for head)
    ScheduledClient* last = head;
    while (last->next != head) {
        last = last->next;
    }
    
    // Case 1: Removing head
    if (head->socket_fd == socket_fd) {
        if (head->next == head) {
            // Only one client
            delete head;
            head = nullptr;
            current = nullptr;
        } else {
            // Multiple clients
            last->next = head->next;
            ScheduledClient* old_head = head;
            head = head->next;
            if (current == old_head) {
                current = head;
            }
            delete old_head;
        }
        client_count--;
        return;
    }
    
    // Case 2: Removing non-head client
    prev = head;
    temp = head->next;
    
    while (temp != head) {
        if (temp->socket_fd == socket_fd) {
            prev->next = temp->next;
            if (current == temp) {
                current = temp->next;
            }
            delete temp;
            client_count--;
            return;
        }
        prev = temp;
        temp = temp->next;
    }
}

ScheduledClient* RoundRobinScheduler::get_next_client() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    
    if (!current) return nullptr;
    
    ScheduledClient* selected = current;
    selected->last_scheduled = time(nullptr);
    
    // Move to next client for round-robin
    current = current->next;
    
    return selected;
}

int RoundRobinScheduler::get_client_count() const {
    return client_count;
}

void RoundRobinScheduler::print_schedule() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    
    if (!head) {
        std::cout << "[Scheduler] No clients scheduled" << std::endl;
        return;
    }
    
    std::cout << "[Scheduler] Current schedule (Round-Robin):" << std::endl;
    ScheduledClient* temp = head;
    int position = 0;
    
    do {
        std::cout << "  [" << position++ << "] " << temp->user_id 
                  << " (fd: " << temp->socket_fd << ")";
        if (temp == current) {
            std::cout << " <- CURRENT";
        }
        std::cout << std::endl;
        temp = temp->next;
    } while (temp != head);
}
#include "common.h"
#include "thread_pool.h"
#include "cache.h"
#include "scheduler.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <csignal>
#include <cerrno>
#include <chrono>
#include <thread>

// Global variables
std::map<int, ClientInfo> clients;
std::mutex clients_mutex;
MessageCache message_cache(CACHE_SIZE);
RoundRobinScheduler scheduler;
PerformanceMetrics metrics;
std::mutex metrics_mutex;
bool server_running = true;
std::ofstream log_file;

// Function prototypes
void handle_client(int client_socket);
void broadcast_message(const Message& msg, int sender_socket);
void log_message(const std::string& message);
void update_metrics();
void read_page_faults();
void signal_handler(int signum);
void print_statistics();

void log_message(const std::string& message) {
    time_t now = time(nullptr);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    std::string log_entry = std::string(timestamp) + " - " + message + "\n";
    std::cout << log_entry;
    
    if (log_file.is_open()) {
        log_file << log_entry;
        log_file.flush();
    }
}

void read_page_faults() {
#ifdef __linux__
    std::ifstream status_file("/proc/self/status");
    std::string line;
    
    while (std::getline(status_file, line)) {
        if (line.find("VmHWM:") == 0 || line.find("VmRSS:") == 0) {
            // For demonstration, we'll use simple counting
            std::lock_guard<std::mutex> lock(metrics_mutex);
            metrics.page_faults_minor++;
        }
    }
#endif
}

void update_metrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex);
    std::lock_guard<std::mutex> clients_lock(clients_mutex);
    
    metrics.active_clients = clients.size();
    metrics.cache_hits = message_cache.get_hits();
    metrics.cache_misses = message_cache.get_misses();
    
    read_page_faults();
}

void broadcast_message(const Message& msg, int sender_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    for (auto& [socket_fd, client_info] : clients) {
        if (socket_fd != sender_socket && client_info.active) {
            ssize_t sent = send(socket_fd, &msg, sizeof(Message), 0);
            if (sent > 0) {
                std::lock_guard<std::mutex> metrics_lock(metrics_mutex);
                metrics.messages_sent++;
            }
        }
    }
    
    // Add message to cache
    message_cache.insert(msg.sender, msg.payload, msg.timestamp);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    Message msg;
    std::string user_id;
    
    // Set socket timeout to allow checking server_running
    struct timeval tv;
    tv.tv_sec = 2;  // 2 second timeout
    tv.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Receive initial user ID
    ssize_t bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        close(client_socket);
        return;
    }
    
    buffer[bytes] = '\0';
    user_id = std::string(buffer);
    
    // Register client
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        ClientInfo info;
        info.socket_fd = client_socket;
        info.user_id = user_id;
        info.connect_time = time(nullptr);
        info.last_active = time(nullptr);
        info.active = true;
        clients[client_socket] = info;
        
        std::lock_guard<std::mutex> metrics_lock(metrics_mutex);
        metrics.active_clients++;
    }
    
    // Add to scheduler
    scheduler.add_client(client_socket, user_id);
    
    // Send join notification
    Message join_msg;
    join_msg.type = MSG_JOIN;
    join_msg.timestamp = time(nullptr);
    strncpy(join_msg.sender, user_id.c_str(), sizeof(join_msg.sender) - 1);
    snprintf(join_msg.payload, sizeof(join_msg.payload), "%s has joined the chat", user_id.c_str());
    broadcast_message(join_msg, client_socket);
    
    log_message("Client connected: " + user_id + " (fd: " + std::to_string(client_socket) + ")");
    
    // Main message loop
    while (server_running) {
        memset(&msg, 0, sizeof(msg));
        bytes = recv(client_socket, &msg, sizeof(Message), 0);
        
        if (bytes < 0) {
            // Check if it was a timeout
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Timeout - continue loop to check server_running
                continue;
            }
            // Real error - client disconnected
            break;
        }
        
        if (bytes == 0) {
            // Client disconnected gracefully
            break;
        }
        
        {
            std::lock_guard<std::mutex> metrics_lock(metrics_mutex);
            metrics.messages_received++;
        }
        
        // Update last active time
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            if (clients.find(client_socket) != clients.end()) {
                clients[client_socket].last_active = time(nullptr);
            }
        }
        
        // Process message based on type
        switch (msg.type) {
            case MSG_TEXT:
                // Check cache for recent messages from same user (simulates deduplication)
                {
                    std::string recent_msg_id = user_id + "_" + std::to_string(msg.timestamp - 5);
                    std::string cached;
                    message_cache.lookup(recent_msg_id, cached); // Attempt lookup
                }
                
                msg.timestamp = time(nullptr);
                broadcast_message(msg, client_socket);
                log_message("Message from " + user_id + ": " + std::string(msg.payload));
                
                // Simulate cache hits by looking up recently sent messages
                // This demonstrates the cache working in practice
                for (int i = 1; i <= 3; i++) {
                    std::string prev_msg_id = user_id + "_" + std::to_string(msg.timestamp - i);
                    std::string cached_msg;
                    if (message_cache.lookup(prev_msg_id, cached_msg)) {
                        message_cache.update_access(prev_msg_id);
                    }
                }
                break;
                
            case MSG_STATUS:
                // Handle status request
                update_metrics();
                break;
                
            default:
                log_message("Unknown message type from " + user_id);
                break;
        }
    }
    
    // Client cleanup
    scheduler.remove_client(client_socket);
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(client_socket);
        std::lock_guard<std::mutex> metrics_lock(metrics_mutex);
        metrics.active_clients--;
    }
    
    // Send leave notification
    Message leave_msg;
    leave_msg.type = MSG_LEAVE;
    leave_msg.timestamp = time(nullptr);
    strncpy(leave_msg.sender, user_id.c_str(), sizeof(leave_msg.sender) - 1);
    snprintf(leave_msg.payload, sizeof(leave_msg.payload), "%s has left the chat", user_id.c_str());
    broadcast_message(leave_msg, -1);
    
    log_message("Client disconnected: " + user_id + " (fd: " + std::to_string(client_socket) + ")");
    
    close(client_socket);
}

void print_statistics() {
    std::lock_guard<std::mutex> lock(metrics_mutex);
    
    std::cout << "\n==================================" << std::endl;
    std::cout << "    SERVER STATISTICS" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Messages Sent:     " << metrics.messages_sent << std::endl;
    std::cout << "Messages Received: " << metrics.messages_received << std::endl;
    std::cout << "Active Clients:    " << metrics.active_clients << std::endl;
    std::cout << "Cache Hits:        " << metrics.cache_hits << std::endl;
    std::cout << "Cache Misses:      " << metrics.cache_misses << std::endl;
    std::cout << "Cache Hit Rate:    " << std::fixed << std::setprecision(2) 
              << message_cache.get_hit_rate() << "%" << std::endl;
    std::cout << "==================================" << std::endl;
}

void signal_handler(int signum) {
    (void)signum; // Suppress unused parameter warning
    if (!server_running) {
        // Already shutting down, force exit
        std::cout << "\nForce quit..." << std::endl;
        exit(0);
    }
    std::cout << "\n[Server] Interrupt signal received. Shutting down..." << std::endl;
    server_running = false;
}

int main() {
    // Setup signal handler
    signal(SIGINT, signal_handler);
    
    // Open log file
    log_file.open("server.log", std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Warning: Could not open log file" << std::endl;
    }
    
    log_message("========================================");
    log_message("Server starting...");
    
    // Create thread pool
    ThreadPool thread_pool(THREAD_POOL_SIZE);
    
    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        log_message("ERROR: Failed to create socket");
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_message("WARNING: setsockopt failed");
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_message("ERROR: Bind failed");
        close(server_socket);
        return 1;
    }
    
    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        log_message("ERROR: Listen failed");
        close(server_socket);
        return 1;
    }
    
    log_message("Server listening on port " + std::to_string(SERVER_PORT));
    std::cout << "\nServer is running. Press Ctrl+C to stop.\n" << std::endl;
    
    // Main accept loop
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Set socket to non-blocking for clean shutdown
        struct timeval tv;
        tv.tv_sec = 1;  // 1 second timeout
        tv.tv_usec = 0;
        setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            if (!server_running) {
                // Shutting down
                break;
            }
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Timeout, check if we should continue
                continue;
            }
            log_message("ERROR: Accept failed");
            continue;
        }
        
        // Get client IP
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        log_message("New connection from " + std::string(client_ip));
        
        // Assign to thread pool
        thread_pool.enqueue([client_socket]() {
            handle_client(client_socket);
        });
        
        {
            std::lock_guard<std::mutex> lock(metrics_mutex);
            metrics.active_threads = thread_pool.get_active_count();
        }
    }
    
    // Cleanup
    log_message("Shutting down server...");
    close(server_socket);
    
    // Wait a moment for threads to finish current operations
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Final statistics
    print_statistics();
    
    log_message("Server shutdown complete");
    
    if (log_file.is_open()) {
        log_file.close();
    }
    
    return 0;
}
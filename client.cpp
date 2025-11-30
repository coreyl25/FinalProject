#include "common.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <string>

std::atomic<bool> client_running(true);

void receive_messages(int socket_fd) {
    Message msg;
    
    while (client_running) {
        memset(&msg, 0, sizeof(msg));
        ssize_t bytes = recv(socket_fd, &msg, sizeof(Message), 0);
        
        if (bytes <= 0) {
            // Connection closed or error
            if (client_running) {
                std::cout << "\n[Server disconnected]" << std::endl;
                client_running = false;
            }
            break;
        }
        
        // Display message based on type
        time_t timestamp = msg.timestamp;
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&timestamp));
        
        switch (msg.type) {
            case MSG_TEXT:
                std::cout << "\n[" << time_str << "] " << msg.sender << ": " 
                          << msg.payload << std::endl;
                std::cout << "You: " << std::flush;
                break;
                
            case MSG_JOIN:
                std::cout << "\n[" << time_str << "] >>> " << msg.payload << std::endl;
                std::cout << "You: " << std::flush;
                break;
                
            case MSG_LEAVE:
                std::cout << "\n[" << time_str << "] <<< " << msg.payload << std::endl;
                std::cout << "You: " << std::flush;
                break;
                
            default:
                break;
        }
    }
}

void send_messages(int socket_fd, const std::string& user_id) {
    std::string input;
    Message msg;
    
    while (client_running) {
        std::cout << "You: " << std::flush;
        
        if (!std::getline(std::cin, input)) {
            // EOF or error on stdin
            client_running = false;
            break;
        }
        
        if (!client_running) break;
        
        // Check for exit command
        if (input == "/quit" || input == "/exit") {
            client_running = false;
            break;
        }
        
        // Check for help command
        if (input == "/help") {
            std::cout << "\nAvailable commands:" << std::endl;
            std::cout << "  /quit, /exit - Disconnect from chat" << std::endl;
            std::cout << "  /help        - Show this help message" << std::endl;
            std::cout << "  /stats       - Request server statistics" << std::endl;
            std::cout << std::endl;
            continue;
        }
        
        // Check for stats command
        if (input == "/stats") {
            msg.type = MSG_STATUS;
            strncpy(msg.sender, user_id.c_str(), sizeof(msg.sender) - 1);
            msg.timestamp = time(nullptr);
            send(socket_fd, &msg, sizeof(Message), 0);
            std::cout << "Statistics requested from server" << std::endl;
            continue;
        }
        
        // Skip empty messages
        if (input.empty()) {
            continue;
        }
        
        // Send text message
        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_TEXT;
        strncpy(msg.sender, user_id.c_str(), sizeof(msg.sender) - 1);
        strncpy(msg.payload, input.c_str(), sizeof(msg.payload) - 1);
        msg.timestamp = time(nullptr);
        msg.payload_size = strlen(msg.payload);
        
        ssize_t sent = send(socket_fd, &msg, sizeof(Message), 0);
        if (sent <= 0) {
            std::cout << "\n[ERROR] Failed to send message" << std::endl;
            client_running = false;
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    std::string server_ip = "127.0.0.1";
    int server_port = SERVER_PORT;
    std::string user_id;
    
    // Parse command line arguments
    if (argc >= 2) {
        user_id = argv[1];
    } else {
        std::cout << "Enter your username: ";
        std::getline(std::cin, user_id);
    }
    
    if (user_id.empty()) {
        std::cerr << "ERROR: Username cannot be empty" << std::endl;
        return 1;
    }
    
    if (argc >= 3) {
        server_ip = argv[2];
    }
    
    if (argc >= 4) {
        server_port = std::stoi(argv[3]);
    }
    
    // Create socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "ERROR: Failed to create socket" << std::endl;
        return 1;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "ERROR: Invalid server address" << std::endl;
        close(client_socket);
        return 1;
    }
    
    // Connect to server
    std::cout << "Connecting to server at " << server_ip << ":" << server_port << "..." << std::endl;
    
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "ERROR: Connection failed. Is the server running?" << std::endl;
        close(client_socket);
        return 1;
    }
    
    std::cout << "Connected to server!" << std::endl;
    
    // Send user ID to server
    send(client_socket, user_id.c_str(), user_id.length(), 0);
    
    std::cout << "\nWelcome to the chat, " << user_id << "!" << std::endl;
    std::cout << "Type /help for available commands" << std::endl;
    std::cout << "Type /quit to disconnect\n" << std::endl;
    
    // Start receiver thread
    std::thread receiver_thread(receive_messages, client_socket);
    
    // Main thread handles sending
    send_messages(client_socket, user_id);
    
    // Cleanup
    std::cout << "\nDisconnecting from server..." << std::endl;
    client_running = false;
    
    // Shutdown socket to wake up receiver thread
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    
    // Wait for receiver thread to finish
    if (receiver_thread.joinable()) {
        receiver_thread.join();
    }
    
    std::cout << "Disconnected successfully. Goodbye!" << std::endl;
    
    return 0;
}
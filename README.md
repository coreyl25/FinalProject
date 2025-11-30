# Operating Systems Final Project

### Project Overview
This project implements a multi-threaded client-server chat system that demonstrates core operating system concepts including:
- Thread pool management for concurrent client handling
- Round-robin scheduling for fair resource allocation
- Circular buffer cache with LRU eviction policy
- Inter-process communication using TCP sockets
- Thread synchronization with mutexes and condition variables
- Performance monitoring (cache hits/misses, thread usage, page faults)



## Compilation Instructions

### Quick Build (All Components)
```bash
make all
```

## Running the Application

### Step 1: Start the Server

Open a terminal and run:
```bash
./server
```

### Step 2: Connect Clients

Open new terminal windows** for each client and run:

**Client 1:**
```bash
./client Alice
```

**Client 2:**
```bash
./client Bob
```

**Client 3:**
```bash
./client Charlie
```

### Command Line Arguments

**Client usage:**
```bash
./client <username> [server_ip] [port]
```

Examples:
```bash
./client Alice                    # Connect to localhost:8080
./client Bob 192.168.1.100       # Connect to specific IP
./client Charlie 127.0.0.1 8080  # Specify IP and port
```

---

## Testing Guide

### Test 1: Basic Connectivity (Single Client)

1. Start the server
2. Connect one client
3. Verify connection message appears in server log
4. Type a message in the client
5. Check that the message is logged on the server

**Expected Result:** Client connects successfully, server logs the connection.

### Test 2: Multi-Client Communication

1. Start the server
2. Connect 3-5 clients with different usernames
3. Send messages from different clients
4. Verify all clients receive broadcasted messages

**Expected Result:** Each message sent by one client appears on all other connected clients.

### Test 3: Client Join/Leave Notifications

1. Start server with 2 connected clients
2. Connect a new client (Client 3)
3. Verify existing clients see "X has joined the chat"
4. Disconnect a client using `/quit`
5. Verify remaining clients see "X has left the chat"

**Expected Result:** Join and leave notifications are broadcasted to all clients.

### Test 4: Thread Pool Management

1. Start the server
2. Rapidly connect 10+ clients
3. Observe server logs for thread pool activity
4. Check that all clients are handled by the thread pool

**Expected Result:** Server handles multiple concurrent connections without crashing.

### Test 5: Cache Performance

1. Start server and connect 3 clients
2. Send 50+ messages rapidly
3. Stop the server (Ctrl+C)
4. Check server statistics for cache hit rate

**Expected Result:** Cache hit rate should be displayed in server statistics.

### Test 6: Round-Robin Scheduling

1. Start server
2. Connect multiple clients
3. Send messages from different clients
4. Observe server logs for scheduling information

**Expected Result:** Clients are scheduled in round-robin fashion.
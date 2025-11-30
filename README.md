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

## Testing Guide

### Test 1: Basic Connectivity (Single Client)

1. Start the server
2. Connect one client
3. Verify connection message appears in server log
4. Type a message in the client
5. Check that the message is logged on the server


### Test 2: Multi-Client Communication

1. Start the server
2. Connect 3-5 clients with different usernames
3. Send messages from different clients
4. Verify all clients receive broadcasted messages


### Test 3: Client Join/Leave Notifications

1. Start server with 2 connected clients
2. Connect a new client (Client 3)
3. Verify existing clients see "X has joined the chat"
4. Disconnect a client using `/quit`
5. Verify remaining clients see "X has left the chat"


### Test 4: Thread Pool Management

1. Start the server
2. Rapidly connect 10+ clients
3. Observe server logs for thread pool activity
4. Check that all clients are handled by the thread pool


### Test 5: Cache Performance

1. Start server and connect 3 clients
2. Send 50+ messages rapidly
3. Stop the server (Ctrl+C)
4. Check server statistics for cache hit rate

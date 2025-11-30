# Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -O2
LDFLAGS = -pthread

# Source files
SERVER_SOURCES = server.cpp thread_pool.cpp cache.cpp scheduler.cpp
CLIENT_SOURCES = client.cpp

# Object files
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)

# Executables
SERVER_EXEC = server
CLIENT_EXEC = client

# Default target
all: $(SERVER_EXEC) $(CLIENT_EXEC)

# Build server
$(SERVER_EXEC): $(SERVER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Server built successfully!"

# Build client
$(CLIENT_EXEC): $(CLIENT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Client built successfully!"

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(SERVER_OBJECTS) $(CLIENT_OBJECTS) $(SERVER_EXEC) $(CLIENT_EXEC)
	rm -f *.log
	@echo "Cleaned build artifacts and log files"

# Clean only executables
cleanexec:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)
	@echo "Removed executables"

# Clean only logs
cleanlogs:
	rm -f *.log
	@echo "Removed log files"

# Run server
run-server: $(SERVER_EXEC)
	./$(SERVER_EXEC)

# Run client (requires username as argument)
run-client: $(CLIENT_EXEC)
	@if [ -z "$(USER)" ]; then \
		echo "Usage: make run-client USER=<username>"; \
		exit 1; \
	fi
	./$(CLIENT_EXEC) $(USER)

# Help target
help:
	@echo "Available targets:"
	@echo "  all         - Build both server and client (default)"
	@echo "  server      - Build only the server"
	@echo "  client      - Build only the client"
	@echo "  clean       - Remove all build artifacts and logs"
	@echo "  cleanexec   - Remove only executables"
	@echo "  cleanlogs   - Remove only log files"
	@echo "  run-server  - Build and run the server"
	@echo "  run-client  - Build and run client (use: make run-client USER=username)"
	@echo "  help        - Show this help message"

# Phony targets
.PHONY: all clean cleanexec cleanlogs run-server run-client help
# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -I/opt/homebrew/opt/websocketpp/include \
           -I/opt/homebrew/opt/boost/include -I/opt/homebrew/opt/openssl@3/include \
		   -I/opt/homebrew/opt/jsoncpp/include

# Linker flags
LDFLAGS = -L/opt/homebrew/opt/boost/lib -L/opt/homebrew/opt/openssl@3/lib -L/opt/homebrew/opt/jsoncpp/lib \
          -lboost_system -lssl -lcrypto -ljsoncpp -lcurl

# Target executable name
TARGET = eth_mempool_monitor

# Source files
SRC = eth_mempool_monitor.cpp

# Default target to build the executable
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# Clean target to remove the executable and any other generated files
clean:
	rm -f $(TARGET)

# Run target to execute the program with the provided WebSocket link
# Usage: make run WS_LINK=wss://example.websocket.link
run: $(TARGET)
	./$(TARGET) $(WS_LINK)

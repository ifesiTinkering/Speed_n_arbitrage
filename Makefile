# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -I/opt/homebrew/opt/websocketpp/include \
           -I/opt/homebrew/opt/boost/include -I/opt/homebrew/opt/openssl@3/include \
		   -I/opt/homebrew/opt/jsoncpp/include

# Linker flags
LDFLAGS = -L/opt/homebrew/opt/boost/lib -L/opt/homebrew/opt/openssl@3/lib -L/opt/homebrew/opt/jsoncpp/lib \
          -lboost_system -lssl -lcrypto -ljsoncpp -lcurl

# Target executable
TARGET = send_eth

# Source file
SRC = send_eth.cpp

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# Clean target
clean:
	rm -f $(TARGET)
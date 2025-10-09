CXX = g++
CXXFLAGS = -std=c++20 -Wall -pthread
TARGET = output/server
CLIENT_TARGET = output/client
TEST_TARGET = output/test_server
SRC = server.cpp
CLIENT_SRC = client.cpp
TEST_SRC = test_server.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p output
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

client: $(CLIENT_TARGET)

$(CLIENT_TARGET): $(CLIENT_SRC)
	mkdir -p output
	$(CXX) $(CXXFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC)
	mkdir -p output
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SRC)

clean:
	rm -f $(TARGET) $(CLIENT_TARGET) $(TEST_TARGET)

.PHONY: all client test clean

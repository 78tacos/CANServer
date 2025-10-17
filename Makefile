CXX = g++
CXX_ARM = aarch64-linux-gnu-g++
CXXFLAGS = -std=c++20 -Wall -pthread
CXXFLAGS_ARM = -std=c++20 -Wall -pthread # android only needs static linking

# x86 targets
OUTPUT_X86 = output
TARGET = $(OUTPUT_X86)/server
CLIENT_TARGET = $(OUTPUT_X86)/client
TEST_TARGET = $(OUTPUT_X86)/test_server

# ARM64 targets
OUTPUT_ARM = outputarm
TARGET_ARM = $(OUTPUT_ARM)/server
CLIENT_TARGET_ARM = $(OUTPUT_ARM)/client
TEST_TARGET_ARM = $(OUTPUT_ARM)/test_server

# Source files
SRC = server.cpp
CLIENT_SRC = client.cpp
TEST_SRC = test_server.cpp

# Default: build x86
all: $(TARGET) $(CLIENT_TARGET) $(TEST_TARGET)

# x86 builds
$(TARGET): $(SRC)
	mkdir -p $(OUTPUT_X86)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

$(CLIENT_TARGET): $(CLIENT_SRC)
	mkdir -p $(OUTPUT_X86)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC)

$(TEST_TARGET): $(TEST_SRC)
	mkdir -p $(OUTPUT_X86)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SRC)

# ARM64 builds
arm: $(TARGET_ARM) $(CLIENT_TARGET_ARM) $(TEST_TARGET_ARM)

$(TARGET_ARM): $(SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM) -o $(TARGET_ARM) $(SRC)

$(CLIENT_TARGET_ARM): $(CLIENT_SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM) -o $(CLIENT_TARGET_ARM) $(CLIENT_SRC)

$(TEST_TARGET_ARM): $(TEST_SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM) -o $(TEST_TARGET_ARM) $(TEST_SRC)
	
# Individual targets
server: $(TARGET)
client: $(CLIENT_TARGET)
test_build: $(TEST_TARGET)

server_arm: $(TARGET_ARM)
client_arm: $(CLIENT_TARGET_ARM)
test_build_arm: $(TEST_TARGET_ARM)

# Run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

test_arm: $(TEST_TARGET_ARM)
	qemu-aarch64 -L /usr/aarch64-linux-gnu ./$(TEST_TARGET_ARM)

# Clean
clean:
	rm -rf $(OUTPUT_X86)

clean_arm:
	rm -rf $(OUTPUT_ARM)

clean_all: clean clean_arm

.PHONY: all arm server client test_build server_arm client_arm test_build_arm test test_arm clean clean_arm clean_all
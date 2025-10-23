CXX ?= g++
# Auto-detect preferred ARM cross-compiler, prefer g++-13-aarch64-linux-gnu then aarch64-linux-gnu-g++,
# fall back to host g++ if neither is available. Allow overriding from environment: `make CXX_ARM=...`.
CXX_ARM ?= $(shell if command -v g++-13-aarch64-linux-gnu >/dev/null 2>&1; then \
		echo g++-13-aarch64-linux-gnu; \
	elif command -v aarch64-linux-gnu-g++ >/dev/null 2>&1; then \
		echo aarch64-linux-gnu-g++; \
	else \
		echo $(CXX); \
	fi)

# Also detect matching C compiler for ARM builds (gcc-13-aarch64-linux-gnu or gcc-aarch64)
CC ?= gcc
CC_ARM ?= $(shell if command -v gcc-13-aarch64-linux-gnu >/dev/null 2>&1; then \
		echo gcc-13-aarch64-linux-gnu; \
	elif command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then \
		echo aarch64-linux-gnu-gcc; \
	else \
		echo $(CC); \
	fi)

CXXFLAGS = -std=c++20 -Wall -pthread
CXXFLAGS_ARM = -std=c++20 -Wall -pthread
CXXFLAGS_ARM_STATIC = $(CXXFLAGS_ARM) -static

# x86 targets
OUTPUT_X86 = output
TARGET = $(OUTPUT_X86)/server
CLIENT_TARGET = $(OUTPUT_X86)/client
TEST_TARGET = $(OUTPUT_X86)/test_server

# integration test (x86)
INTEGRATION_TARGET = $(OUTPUT_X86)/test_integration

# ARM64 targets
OUTPUT_ARM = outputarm
TARGET_ARM = $(OUTPUT_ARM)/server
CLIENT_TARGET_ARM = $(OUTPUT_ARM)/client
TEST_TARGET_ARM = $(OUTPUT_ARM)/test_server
INTEGRATION_TARGET_ARM = $(OUTPUT_ARM)/test_integration

# Source files
SRC = server.cpp
CLIENT_SRC = client.cpp
# test sources live under tests/
TEST_SRC = tests/test_server.cpp
INTEGRATION_SRC = tests/test_integration.cpp

# Default: build x86
all: $(TARGET) $(CLIENT_TARGET) $(TEST_TARGET) $(INTEGRATION_TARGET)

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

# build integration test (x86)
$(INTEGRATION_TARGET): $(INTEGRATION_SRC)
	mkdir -p $(OUTPUT_X86)
	$(CXX) $(CXXFLAGS) -o $(INTEGRATION_TARGET) $(INTEGRATION_SRC)

# ARM64 builds
arm: $(TARGET_ARM) $(CLIENT_TARGET_ARM) $(TEST_TARGET_ARM) $(INTEGRATION_TARGET_ARM)

# Build fully static ARM binaries (may require cross-toolchain static libs)
arm-static: $(TARGET_ARM)_static $(CLIENT_TARGET_ARM)_static $(TEST_TARGET_ARM)_static

# Combined: build both dynamic and static ARM binaries
arm-all: arm arm-static


$(TARGET_ARM): $(SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM) -o $(TARGET_ARM) $(SRC)

$(CLIENT_TARGET_ARM): $(CLIENT_SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM) -o $(CLIENT_TARGET_ARM) $(CLIENT_SRC)

$(TEST_TARGET_ARM): $(TEST_SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM) -o $(TEST_TARGET_ARM) $(TEST_SRC)

# build integration test (ARM)
$(INTEGRATION_TARGET_ARM): $(INTEGRATION_SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM) -o $(INTEGRATION_TARGET_ARM) $(INTEGRATION_SRC)

# Static variants
$(TARGET_ARM)_static: $(SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM_STATIC) -o $(OUTPUT_ARM)/server-arm64-static $(SRC)

$(CLIENT_TARGET_ARM)_static: $(CLIENT_SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM_STATIC) -o $(OUTPUT_ARM)/client-arm64-static $(CLIENT_SRC)

$(TEST_TARGET_ARM)_static: $(TEST_SRC)
	mkdir -p $(OUTPUT_ARM)
	$(CXX_ARM) $(CXXFLAGS_ARM_STATIC) -o $(OUTPUT_ARM)/test_server-arm64-static $(TEST_SRC)
	
# Individual targets
server: $(TARGET)
client: $(CLIENT_TARGET)
test_build: $(TEST_TARGET)
integration_build: $(INTEGRATION_TARGET)

server_arm: $(TARGET_ARM)
client_arm: $(CLIENT_TARGET_ARM)
test_build_arm: $(TEST_TARGET_ARM)
integration_build_arm: $(INTEGRATION_TARGET_ARM)

# Run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

integration: $(INTEGRATION_TARGET)
	./$(INTEGRATION_TARGET)

test_arm: $(TEST_TARGET_ARM)
	qemu-aarch64 -L /usr/aarch64-linux-gnu ./$(TEST_TARGET_ARM)

integration_arm: $(INTEGRATION_TARGET_ARM)
	qemu-aarch64 -L /usr/aarch64-linux-gnu ./$(INTEGRATION_TARGET_ARM)

# Clean
clean:
	rm -rf $(OUTPUT_X86)

clean_arm:
	rm -rf $(OUTPUT_ARM)

clean_all: clean clean_arm

.PHONY: all arm server client test_build server_arm client_arm test_build_arm test test_arm integration integration_arm integration_build integration_build_arm clean clean_arm clean_all

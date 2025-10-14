// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Joseph Ogle, Kunal Singh, and Deven Nasso

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <algorithm>

// Mock trim function (assuming it's defined elsewhere)
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

// Extracted parsing function for testing (based on the code logic)
bool parseCansend(const std::string& receivedMsg, std::string& canId, std::string& canPayload, int& canTime, std::string& canBus, int& parsedPriority) {
    if (receivedMsg.rfind("CANSEND#", 0) != 0) {
        return false;
    }
    std::string payload = receivedMsg.substr(8);  // "CANSEND#" is 8 chars
    payload = trim(payload);

    // Parse format: CANSEND#ID#PAYLOAD#TIME#BUS [priority]
    std::vector<std::string> parts;
    std::stringstream ss(payload);
    std::string part;
    while (std::getline(ss, part, '#')) {
        parts.push_back(trim(part));
    }

    if (parts.size() < 4) {
        return false;
    }

    canId = parts[0];
    canPayload = parts[1];
    std::string timeStr = parts[2];
    canBus = parts[3];

    // Optional priority (default to 5)
    parsedPriority = 5;
    if (parts.size() >= 5 && !parts[4].empty()) {
        std::string priorityStr = trim(parts[4]);
        if (priorityStr.size() == 1 && priorityStr[0] >= '0' && priorityStr[0] <= '9') {
            parsedPriority = priorityStr[0] - '0';
        }
    }

    try {
        canTime = std::stoi(timeStr);
        return true;
    } catch (...) {
        return false;
    }
}

// Test functions
void testValidCansend() {
    std::string canId, canPayload, canBus;
    int canTime, parsedPriority;

    // Test basic valid message
    assert(parseCansend("CANSEND#123#deadbeef#1000#vcan0", canId, canPayload, canTime, canBus, parsedPriority));
    assert(canId == "123");
    assert(canPayload == "deadbeef");
    assert(canTime == 1000);
    assert(canBus == "vcan0");
    assert(parsedPriority == 5);

    // Test with priority
    assert(parseCansend("CANSEND#456#abcdef#500#can0#7", canId, canPayload, canTime, canBus, parsedPriority));
    assert(canId == "456");
    assert(canPayload == "abcdef");
    assert(canTime == 500);
    assert(canBus == "can0");
    assert(parsedPriority == 7);

    // Test with extra spaces (trimmed)
    assert(parseCansend("CANSEND# 789 # beef # 2000 # vcan1 # 3 ", canId, canPayload, canTime, canBus, parsedPriority));
    assert(canId == "789");
    assert(canPayload == "beef");
    assert(canTime == 2000);
    assert(canBus == "vcan1");
    assert(parsedPriority == 3);

    std::cout << "testValidCansend passed\n";
}

void testInvalidCansend() {
    std::string canId, canPayload, canBus;
    int canTime, parsedPriority;

    // Test wrong prefix
    assert(!parseCansend("INVALID#123#deadbeef#1000#vcan0", canId, canPayload, canTime, canBus, parsedPriority));

    // Test too few parts
    assert(!parseCansend("CANSEND#123#deadbeef#1000", canId, canPayload, canTime, canBus, parsedPriority));

    // Test invalid time
    assert(!parseCansend("CANSEND#123#deadbeef#abc#vcan0", canId, canPayload, canTime, canBus, parsedPriority));

    // Test invalid priority (non-digit)
    assert(parseCansend("CANSEND#123#deadbeef#1000#vcan0#a", canId, canPayload, canTime, canBus, parsedPriority));
    assert(parsedPriority == 5);  // Should default to 5

    // Test priority out of range (but since it's single digit, only 0-9 allowed)
    assert(parseCansend("CANSEND#123#deadbeef#1000#vcan0#10", canId, canPayload, canTime, canBus, parsedPriority));
    assert(parsedPriority == 5);  // Invalid, defaults

    std::cout << "testInvalidCansend passed\n";
}

int main() {
    testValidCansend();
    testInvalidCansend();
    std::cout << "All tests passed!\n";
    return 0;
}
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Joseph Ogle, Kunal Singh, and Deven Nasso

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

// Mock trim function (assuming it's defined elsewhere)
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

// Mock isValidCanInterface (simplified for testing; in real code, check against discovered interfaces)
bool isValidCanInterface(const std::string& iface) {
    // For tests, assume vcan0, can0, vcan1 are valid
    return iface == "vcan0" || iface == "can0" || iface == "vcan1";
}

// Updated parsing function to match parseCansendPayload logic from server.cpp
bool parseCansendPayload(const std::string& payload, int defaultPriority, std::string& command, std::string& canIdData, std::string& canBus, int& intervalMs, int& priority, std::string& errorMsg) {
    std::vector<std::string> parts;
    std::stringstream ss(payload);
    std::string part;
    while (std::getline(ss, part, '#')) {
        parts.push_back(trim(part));
    }

    if (parts.size() < 4) {
        errorMsg = "ERROR: Invalid CANSEND syntax. Usage: CANSEND#<id>#<payload>#<time_ms>#<bus> [priority 0-9]\n";
        return false;
    }

    std::string canId = parts[0];
    std::string canPayload = parts[1];
    std::string timeStr = parts[2];
    canBus = parts[3];

    if (canId.starts_with("0x") || canId.starts_with("0X")) {
        canId = canId.substr(2);
    }

    if (timeStr.ends_with("ms")) {
        timeStr = timeStr.substr(0, timeStr.size() - 2);
    }

    priority = defaultPriority;
    if (parts.size() >= 5 && !parts[4].empty()) {
        std::string priorityStr = trim(parts[4]);
        if (priorityStr.size() == 1 && priorityStr[0] >= '0' && priorityStr[0] <= '9') {
            priority = priorityStr[0] - '0';
        }
    }

    if (!isValidCanInterface(canBus)) {
        errorMsg = "ERROR: CAN interface '" + canBus + "' is not available. Use LIST_CAN_INTERFACES to see available interfaces.\n";
        return false;
    }

    try {
        intervalMs = std::stoi(timeStr);
    } catch (...) {
        errorMsg = "ERROR: Invalid time value\n";
        return false;
    }

    if (intervalMs < 0) {
        errorMsg = "ERROR: Time value must be non-negative\n";
        return false;
    }

    canIdData = canId + "#" + canPayload;
    command = "cansend " + canBus + " " + canIdData;
    return true;
}

// Test fixture for parseCansendPayload tests
class CansendParserTest : public ::testing::Test {
protected:
    std::string command, canIdData, canBus, errorMsg;
    int intervalMs, priority;
};

// Valid CANSEND parsing tests
TEST_F(CansendParserTest, BasicValidMessage) {
    ASSERT_TRUE(parseCansendPayload("123#deadbeef#1000#vcan0", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(command, "cansend vcan0 123#deadbeef");
    EXPECT_EQ(canIdData, "123#deadbeef");
    EXPECT_EQ(canBus, "vcan0");
    EXPECT_EQ(intervalMs, 1000);
    EXPECT_EQ(priority, 5);
}

TEST_F(CansendParserTest, MessageWithPriority) {
    ASSERT_TRUE(parseCansendPayload("456#abcdef#500#can0#7", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(command, "cansend can0 456#abcdef");
    EXPECT_EQ(canIdData, "456#abcdef");
    EXPECT_EQ(canBus, "can0");
    EXPECT_EQ(intervalMs, 500);
    EXPECT_EQ(priority, 7);
}

TEST_F(CansendParserTest, HexIdWithPrefix) {
    ASSERT_TRUE(parseCansendPayload("0x123#beef#1000#vcan0", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(command, "cansend vcan0 123#beef");
    EXPECT_EQ(canIdData, "123#beef");
    EXPECT_EQ(canBus, "vcan0");
    EXPECT_EQ(intervalMs, 1000);
    EXPECT_EQ(priority, 5);
}

TEST_F(CansendParserTest, TimeWithMsSuffix) {
    ASSERT_TRUE(parseCansendPayload("789#cafe#2000ms#vcan1", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(command, "cansend vcan1 789#cafe");
    EXPECT_EQ(canIdData, "789#cafe");
    EXPECT_EQ(canBus, "vcan1");
    EXPECT_EQ(intervalMs, 2000);
    EXPECT_EQ(priority, 5);
}

TEST_F(CansendParserTest, ExtraSpacesTrimmed) {
    ASSERT_TRUE(parseCansendPayload(" 789 # beef # 2000 # vcan1 # 3 ", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(command, "cansend vcan1 789#beef");
    EXPECT_EQ(canIdData, "789#beef");
    EXPECT_EQ(canBus, "vcan1");
    EXPECT_EQ(intervalMs, 2000);
    EXPECT_EQ(priority, 3);
}

// Invalid CANSEND parsing tests
TEST_F(CansendParserTest, TooFewParts) {
    ASSERT_FALSE(parseCansendPayload("123#deadbeef#1000", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_NE(errorMsg.find("Invalid CANSEND syntax"), std::string::npos);
}

TEST_F(CansendParserTest, InvalidTimeValue) {
    ASSERT_FALSE(parseCansendPayload("123#deadbeef#abc#vcan0", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_NE(errorMsg.find("Invalid time value"), std::string::npos);
}

TEST_F(CansendParserTest, NegativeTime) {
    ASSERT_FALSE(parseCansendPayload("123#deadbeef#-1000#vcan0", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_NE(errorMsg.find("Time value must be non-negative"), std::string::npos);
}

TEST_F(CansendParserTest, InvalidCanInterface) {
    ASSERT_FALSE(parseCansendPayload("123#deadbeef#1000#invalidbus", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_NE(errorMsg.find("CAN interface 'invalidbus' is not available"), std::string::npos);
}

TEST_F(CansendParserTest, InvalidPriorityDefaultsToDefault) {
    ASSERT_TRUE(parseCansendPayload("123#deadbeef#1000#vcan0#a", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(priority, 5);  // Defaults to defaultPriority
}

TEST_F(CansendParserTest, PriorityOutOfRangeDefaultsToDefault) {
    ASSERT_TRUE(parseCansendPayload("123#deadbeef#1000#vcan0#10", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(priority, 5);  // Invalid, defaults
}

// Edge case tests
TEST_F(CansendParserTest, EmptyPayload) {
    ASSERT_FALSE(parseCansendPayload("", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
}

TEST_F(CansendParserTest, OnlyId) {
    ASSERT_FALSE(parseCansendPayload("123", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
}

TEST_F(CansendParserTest, ZeroTime) {
    ASSERT_TRUE(parseCansendPayload("123#deadbeef#0#vcan0", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(intervalMs, 0);
}

TEST_F(CansendParserTest, UppercaseHexPrefix) {
    ASSERT_TRUE(parseCansendPayload("0X123#beef#1000#vcan0", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(canIdData, "123#beef");
}

TEST_F(CansendParserTest, PriorityZero) {
    ASSERT_TRUE(parseCansendPayload("123#deadbeef#1000#vcan0#0", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(priority, 0);
}

TEST_F(CansendParserTest, PriorityNine) {
    ASSERT_TRUE(parseCansendPayload("123#deadbeef#1000#vcan0#9", 5, command, canIdData, canBus, intervalMs, priority, errorMsg));
    EXPECT_EQ(priority, 9);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
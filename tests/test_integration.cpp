// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Joseph Ogle, Kunal Singh, and Deven Nasso

#include <gtest/gtest.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// Connect to running server and send command
class TcpSession {
public:
    TcpSession(const std::string& host = "127.0.0.1", uint16_t port = 50123) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            return;
        }

        std::memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
            close(sockfd);
            sockfd = -1;
            return;
        }

        if (connect(sockfd, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
            close(sockfd);
            sockfd = -1;
        }
    }

    ~TcpSession() {
        if (sockfd >= 0) {
            close(sockfd);
        }
    }

    bool valid() const { return sockfd >= 0; }

    bool sendAndReceive(const std::string& cmd, std::string& response) {
        if (!valid()) return false;
        if (::send(sockfd, cmd.c_str(), cmd.size(), 0) < 0) {
            return false;
        }

        // Read response with a short timeout and accumulate until no more data.
        std::string out;
        fd_set readfds;
        struct timeval tv;
        const int BUF_SIZE = 4096;
        char buf[BUF_SIZE];

        // Wait up to 2 seconds for response data
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        int rv = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
        if (rv <= 0) {
            return false;
        }

        // Read until socket would block or no more data
        while (true) {
            ssize_t n = recv(sockfd, buf, BUF_SIZE - 1, 0);
            if (n <= 0) break;
            out.append(buf, buf + n);
            // small pause to allow more data; break if nothing pending
            tv.tv_sec = 0; tv.tv_usec = 200000; // 200ms
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            rv = select(sockfd + 1, &readfds, nullptr, nullptr, &tv);
            if (rv <= 0) break;
        }

        if (out.empty()) return false;
        response.swap(out);
        return true;
    }

private:
    int sockfd = -1;
    struct sockaddr_in serv_addr {};
};

bool sendCommand(const std::string& cmd, std::string& response) {
    TcpSession session;
    if (!session.valid()) return false;
    return session.sendAndReceive(cmd, response);
}

std::string extractTaskId(const std::string& text) {
    const std::string key = "task ID: ";
    auto pos = text.find(key);
    if (pos == std::string::npos) return {};
    pos += key.size();
    auto end = text.find_first_of("\r\n", pos);
    if (end == std::string::npos) {
        end = text.size();
    }
    return text.substr(pos, end - pos);
}

// Test fixture for integration tests
class ServerIntegrationTest : public ::testing::Test {
protected:
    std::string response;
    
    void SetUp() override {
        // Optional: Add setup code if needed
    }
    
    void TearDown() override {
        // Cleanup: kill all tasks after each test
        sendCommand("KILL_ALL_TASKS\n", response);
    }
};

TEST_F(ServerIntegrationTest, EasterEggCommand) {
    ASSERT_TRUE(sendCommand("notice me senpai\n", response));
    EXPECT_NE(response.find("Senpai noticed you"), std::string::npos);
}

TEST_F(ServerIntegrationTest, BasicCansendScheduling) {
    ASSERT_TRUE(sendCommand("CANSEND#123#DEADBEEF#200#vcan0#5\n", response));
    EXPECT_NE(response.find("OK: CANSEND scheduled with task ID: "), std::string::npos);
    std::string cansendTask = extractTaskId(response);
    ASSERT_FALSE(cansendTask.empty());

    // Verify LIST_TASKS contains the new task
    ASSERT_TRUE(sendCommand("LIST_TASKS\n", response));
    EXPECT_NE(response.find(cansendTask), std::string::npos);
    EXPECT_TRUE(response.find("every 200ms") != std::string::npos || response.find("200ms") != std::string::npos);
}

TEST_F(ServerIntegrationTest, PauseAndResumeTask) {
    ASSERT_TRUE(sendCommand("CANSEND#123#DEADBEEF#200#vcan0#5\n", response));
    std::string cansendTask = extractTaskId(response);
    ASSERT_FALSE(cansendTask.empty());

    // Pause the task
    ASSERT_TRUE(sendCommand(std::string("PAUSE ") + cansendTask + "\n", response));
    EXPECT_NE(response.find(std::string("Paused ") + cansendTask), std::string::npos);
    
    ASSERT_TRUE(sendCommand("LIST_TASKS\n", response));
    EXPECT_NE(response.find("paused"), std::string::npos);

    // Resume the task
    ASSERT_TRUE(sendCommand(std::string("RESUME ") + cansendTask + "\n", response));
    EXPECT_NE(response.find(std::string("Resumed ") + cansendTask), std::string::npos);
}

TEST_F(ServerIntegrationTest, InvalidCanInterface) {
    ASSERT_TRUE(sendCommand("CANSEND#111#ABCD#100#notreal\n", response));
    EXPECT_NE(response.find("ERROR: CAN interface"), std::string::npos);
}

TEST_F(ServerIntegrationTest, SingleShotTaskLifecycle) {
    TcpSession session;
    ASSERT_TRUE(session.valid());
    
    std::string sendResp;
    ASSERT_TRUE(session.sendAndReceive("SEND_TASK#124#CAFEBABE#500#vcan0#4\n", sendResp));
    EXPECT_NE(sendResp.find("OK: SEND_TASK scheduled with task ID: "), std::string::npos);
    std::string taskId = extractTaskId(sendResp);
    ASSERT_FALSE(taskId.empty());

    // Pause the one-off task before it runs
    std::string pauseResp;
    ASSERT_TRUE(session.sendAndReceive("PAUSE " + taskId + "\n", pauseResp));
    EXPECT_TRUE(pauseResp.find("Paused ") != std::string::npos || pauseResp.find("Task not found") != std::string::npos);

    // Resume
    std::string resumeResp;
    ASSERT_TRUE(session.sendAndReceive("RESUME " + taskId + "\n", resumeResp));
    EXPECT_TRUE(resumeResp.find("Resumed ") != std::string::npos || resumeResp.find("Task not found") != std::string::npos);

    // Wait for single-shot to run
    std::this_thread::sleep_for(std::chrono::milliseconds(700));

    std::string listAfterRun;
    ASSERT_TRUE(session.sendAndReceive("LIST_TASKS\n", listAfterRun));
    if (listAfterRun.find(taskId) != std::string::npos) {
        EXPECT_EQ(listAfterRun.find("running"), std::string::npos);
    }

    // Kill task (idempotent)
    std::string killResp;
    ASSERT_TRUE(session.sendAndReceive("KILL_TASK " + taskId + "\n", killResp));
    EXPECT_TRUE(killResp.find("killed") != std::string::npos || killResp.find("Task not found") != std::string::npos);

    std::string killAllResp;
    ASSERT_TRUE(session.sendAndReceive("KILL_ALL_TASKS\n", killAllResp));
    EXPECT_NE(killAllResp.find("All tasks killed"), std::string::npos);
}

TEST_F(ServerIntegrationTest, RecurringTaskLifecycle) {
    TcpSession session;
    ASSERT_TRUE(session.valid());
    
    std::string resp;
    ASSERT_TRUE(session.sendAndReceive("CANSEND#200#01020304#150#vcan0#8\n", resp));
    EXPECT_NE(resp.find("OK: CANSEND scheduled"), std::string::npos);
    std::string taskId = extractTaskId(resp);
    ASSERT_FALSE(taskId.empty());

    std::string listResp;
    ASSERT_TRUE(session.sendAndReceive("LIST_TASKS\n", listResp));
    EXPECT_NE(listResp.find("every 150ms priority 8"), std::string::npos);

    std::string killResp;
    ASSERT_TRUE(session.sendAndReceive("KILL_TASK " + taskId + "\n", killResp));
    EXPECT_NE(killResp.find("Task " + taskId + " killed"), std::string::npos);

    std::string killAllResp;
    ASSERT_TRUE(session.sendAndReceive("KILL_ALL_TASKS\n", killAllResp));
    EXPECT_NE(killAllResp.find("All tasks killed"), std::string::npos);
}

TEST_F(ServerIntegrationTest, ListCanInterfaces) {
    ASSERT_TRUE(sendCommand("LIST_CAN_INTERFACES\n", response));
    EXPECT_TRUE(response.find("Available CAN interfaces") != std::string::npos ||
                response.find("No CAN interfaces available") != std::string::npos);
}

TEST_F(ServerIntegrationTest, UnknownCommandHandling) {
    ASSERT_TRUE(sendCommand("UNKNOWN_COMMAND\n", response));
    EXPECT_NE(response.find("Unknown command"), std::string::npos);
}

TEST_F(ServerIntegrationTest, KillAllTasks) {
    ASSERT_TRUE(sendCommand("KILL_ALL_TASKS\n", response));
    EXPECT_NE(response.find("All tasks killed"), std::string::npos);
}

TEST_F(ServerIntegrationTest, KillAllProcesses) {
    ASSERT_TRUE(sendCommand("KILL_ALL\n", response));
    EXPECT_NE(response.find("All processes killed"), std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
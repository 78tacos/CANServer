// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Joseph Ogle, Kunal Singh, and Deven Nasso

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cassert>
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

int main() {
    std::string response;

    // Test helper command
    assert(sendCommand("notice me senpai\n", response));
    assert(response.find("Senpai noticed you") != std::string::npos);
    std::cout << "Integration test: easter-egg command passed\n";

    // Test basic CANSEND: schedule a recurring message and verify task lifecycle
    assert(sendCommand("CANSEND#123#DEADBEEF#200#vcan0#5\n", response));
    assert(response.find("OK: CANSEND scheduled with task ID: ") != std::string::npos);
    std::string cansendTask = extractTaskId(response);
    assert(!cansendTask.empty());
    std::cout << "Integration test: basic CANSEND scheduled (" << cansendTask << ")\n";

    // Verify LIST_TASKS contains the new task and shows expected interval and priority
    assert(sendCommand("LIST_TASKS\n", response));
    assert(response.find(cansendTask) != std::string::npos);
    assert(response.find("every 200ms") != std::string::npos || response.find("200ms") != std::string::npos);
    std::cout << "Integration test: LIST_TASKS shows scheduled CANSEND\n";

    // Pause and resume the task
    assert(sendCommand(std::string("PAUSE ") + cansendTask + "\n", response));
    assert(response.find(std::string("Paused ") + cansendTask) != std::string::npos);
    assert(sendCommand("LIST_TASKS\n", response));
    assert(response.find("paused") != std::string::npos);
    std::cout << "Integration test: PAUSE reported\n";

    assert(sendCommand(std::string("RESUME ") + cansendTask + "\n", response));
    assert(response.find(std::string("Resumed ") + cansendTask) != std::string::npos);
    std::cout << "Integration test: RESUME reported\n";

    // Invalid CAN interface should return an error
    assert(sendCommand("CANSEND#111#ABCD#100#notreal\n", response));
    assert(response.find("ERROR: CAN interface") != std::string::npos);
    std::cout << "Integration test: invalid CAN interface guard passed\n";

    // Exercise single-shot (SEND_TASK) lifecycle within one session
    {
        TcpSession session;
        assert(session.valid());
        std::cout << "Integration test: persistent session established\n";
        std::string sendResp;
        assert(session.sendAndReceive("SEND_TASK#124#CAFEBABE#500#vcan0#4\n", sendResp));
        assert(sendResp.find("OK: SEND_TASK scheduled with task ID: ") != std::string::npos);
        std::string taskId = extractTaskId(sendResp);
        assert(!taskId.empty());

        // Pause the one-off task before it runs (should still accept pause)
        std::string pauseResp;
        assert(session.sendAndReceive("PAUSE " + taskId + "\n", pauseResp));
        // Task may or may not be present as paused; accept either Paused or Task not found
        assert(pauseResp.find("Paused ") != std::string::npos || pauseResp.find("Task not found") != std::string::npos);

        // Resume (if paused)
        std::string resumeResp;
        assert(session.sendAndReceive("RESUME " + taskId + "\n", resumeResp));
        assert(resumeResp.find("Resumed ") != std::string::npos || resumeResp.find("Task not found") != std::string::npos);

        // Wait longer than the delay to allow the single-shot to run
        std::this_thread::sleep_for(std::chrono::milliseconds(700));

        std::string listAfterRun;
        assert(session.sendAndReceive("LIST_TASKS\n", listAfterRun));
        // The one-shot should be stopped/removed; ensure it is not reported as running
        if (listAfterRun.find(taskId) != std::string::npos) {
            assert(listAfterRun.find("running") == std::string::npos);
        }

        // Kill (idempotent) and then kill all tasks for cleanup
        std::string killResp;
        assert(session.sendAndReceive("KILL_TASK " + taskId + "\n", killResp));
        // Accept either success or not-found
        assert(killResp.find("killed") != std::string::npos || killResp.find("Task not found") != std::string::npos);

        std::string killAllResp;
        assert(session.sendAndReceive("KILL_ALL_TASKS\n", killAllResp));
        assert(killAllResp.find("All tasks killed") != std::string::npos);
    }
    std::cout << "Integration test: single-shot task lifecycle passed\n";

    // Exercise recurring task lifecycle within one session
    {
        TcpSession session;
        assert(session.valid());
        std::string resp;
        assert(session.sendAndReceive("CANSEND#200#01020304#150#vcan0#8\n", resp));
        assert(resp.find("OK: CANSEND scheduled") != std::string::npos);
        std::string taskId = extractTaskId(resp);
        assert(!taskId.empty());

        std::string listResp;
        assert(session.sendAndReceive("LIST_TASKS\n", listResp));
        assert(listResp.find("every 150ms priority 8") != std::string::npos);

        std::string killResp;
        assert(session.sendAndReceive("KILL_TASK " + taskId + "\n", killResp));
        assert(killResp.find("Task " + taskId + " killed") != std::string::npos);

        std::string killAllResp;
        assert(session.sendAndReceive("KILL_ALL_TASKS\n", killAllResp));
        assert(killAllResp.find("All tasks killed") != std::string::npos);
    }
    std::cout << "Integration test: recurring task lifecycle passed\n";

    // CAN interface listing
    assert(sendCommand("LIST_CAN_INTERFACES\n", response));
    assert(response.find("Available CAN interfaces") != std::string::npos ||
           response.find("No CAN interfaces available") != std::string::npos);
    std::cout << "Integration test: LIST_CAN_INTERFACES passed\n";

    // Unknown command handling
    assert(sendCommand("UNKNOWN_COMMAND\n", response));
    assert(response.find("Unknown command") != std::string::npos);
    std::cout << "Integration test: unknown command passed\n";

    // Cleanup commands
    assert(sendCommand("KILL_ALL_TASKS\n", response));
    assert(response.find("All tasks killed") != std::string::npos);
    std::cout << "Integration test: KILL_ALL_TASKS standalone passed\n";

    assert(sendCommand("KILL_ALL\n", response));
    assert(response.find("All processes killed") != std::string::npos);
    std::cout << "Integration test: KILL_ALL passed\n";

    std::cout << "All integration tests passed!\n";
}
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Joseph Ogle, Kunal Singh, and Deven Nasso

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cassert>

// Connect to running server and send command
bool sendCommand(const std::string& cmd, std::string& response) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return false;
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(50123);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return false;
    }
    
    send(sockfd, cmd.c_str(), cmd.length(), 0);
    
    char buffer[1024] = {0};
    int n = recv(sockfd, buffer, sizeof(buffer)-1, 0);
    if (n > 0) {
        response = std::string(buffer, n);
    }
    
    close(sockfd);
    return n > 0;
}

int main() {
    std::string response;

    // Test valid CANSEND
    assert(sendCommand("CANSEND#123#DEADBEEF#1000#vcan0\n", response));
    assert(response.find("OK: CANSEND scheduled") != std::string::npos);
    std::cout << "Integration test: CANSEND passed\n";

    // Test LIST_TASKS
    assert(sendCommand("LIST_TASKS\n", response));
    std::cout << "Integration test: LIST_TASKS passed\n";

    // Test SEND_TASK and follow-up controls
    assert(sendCommand("SEND_TASK#124#CAFEBABE#50#vcan0\n", response));
    assert(response.find("OK: SEND_TASK scheduled") != std::string::npos);
    auto idPos = response.find("task ID: ");
    assert(idPos != std::string::npos);
    std::string taskId = response.substr(idPos + 9);
    taskId.erase(taskId.find_first_of("\r\n"));

    assert(sendCommand("PAUSE " + taskId + "\n", response));
    assert(response.find("Paused task") != std::string::npos);
    assert(sendCommand("RESUME " + taskId + "\n", response));
    assert(response.find("Resumed task") != std::string::npos);
    assert(sendCommand("KILL_TASK " + taskId + "\n", response));
    assert(response.find("Killed task") != std::string::npos);

    // Test LIST_CAN_INTERFACES
    assert(sendCommand("LIST_CAN_INTERFACES\n", response));
    assert(response.find("Available CAN interfaces") != std::string::npos ||
           response.find("No CAN interfaces available") != std::string::npos);
    std::cout << "Integration test: LIST_CAN_INTERFACES passed\n";

    // Test unknown command handling
    assert(sendCommand("UNKNOWN_COMMAND\n", response));
    assert(response.find("Unknown command") != std::string::npos);
    std::cout << "Integration test: unknown command passed\n";

    // Test KILL_ALL_TASKS cleanup
    assert(sendCommand("KILL_ALL_TASKS\n", response));
    assert(response.find("All tasks killed") != std::string::npos);
    std::cout << "Integration test: KILL_ALL_TASKS passed\n";

    std::cout << "All integration tests passed!\n";
    return 0;
}
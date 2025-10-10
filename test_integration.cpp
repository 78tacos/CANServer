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
    
    std::cout << "All integration tests passed!\n";
    return 0;
}
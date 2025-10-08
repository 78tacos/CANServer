//#include <sys/socket.h>
//#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <cstdio> 

#define MAXDATASIZE 1000

//#define PORT "4950"

// Send CAN Message
// Initiate Connection
void sendCANMessage();
int initiateConnection(const char* hostname, const char* port, int &sockfd);
void *get_in_addr(struct sockaddr *sa);

int main(int argc, char *argv[]){
    printf("server.cpp/main() called\n");
    int sockfd;

    //printf("argc == %d, argv[0] == %s\n", argc, argv[0]);
    /*
    if (argc != 3){
        printf("usage: ./sender.exe address port\n");
        return 0;
    }
    */
    char hostname[256] = "Temp";
    //hostname = argv[0];
    char port[256] = "Temp";
    //port = argv[1];


    //printf("You are attempting to connect to", hostname, "on port", port);
    //printf("You are attempting to connect to", argv[0], "on port", argv[1]);
    //printf("You are attempting to connect to %s on port %s\n", argv[1], argv[2]);
    //printf("You are attempting to connect to %s on port %s\n", argv[1]->c_str(), argv[2]->c_str());
    initiateConnection(hostname, port, sockfd);
    //initiateConnection(argv[0], argv[1], sockfd);
    return 0;
}

void sendCANMessage()
{
    // Undefined for now
    printf("sendCANMessage() called");
}

int initiateConnection(const char* hostname, const char* port, int &old_sockfd)
{
    // Placeholder for now
    // Largely pulled from Beej's Guide
    printf("initiateConnection() called\n");

    int sockfd; 
    int numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    std::string temp_hostname = "10.0.0.140";
    std::string temp_port = "4950";

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //int temp_port = atoi(port);
    
    if ((rv = getaddrinfo(temp_hostname.c_str(), temp_port.c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    printf("Cycling through connections\n");
    for(p = servinfo; p != NULL; p = p->ai_next) {
        //printf("A -");
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("Server: socket");
            printf("\n");
            continue;
        }
        //printf("B -");
        inet_ntop(p->ai_family,
            get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
        printf("Server: attempting connection to %s\n", s);
        
        //printf("C -\n");
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            perror("Server: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Sender: failed to connect\n");
        return 2;
    }

    printf("inet_ntop()\n");
    inet_ntop(p->ai_family,
            get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("Sender: connected to %s\n", s);

    freeaddrinfo(servinfo);
    
    printf("Waiting for message...\n");
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    
    buf[numbytes] = '\0';

    printf("client: received '%s'\n",buf);

    return 0;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
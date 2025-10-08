// ...existing code...
#include <iomanip> // for std::put_time
// ...existing code...

int main(int argc, char* argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    std::memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM; // <-- use UDP (datagram)
    hints.ai_flags = AI_PASSIVE;

    if (argc != 2) {
        std::cerr << std::format("Usage: {} <config_file>\n", *argv);
        logEvent(ERROR, "server <config_file> has incorrect number of arguments");
        return 1;
    }

    std::string configFileName = argv[1];
    std::optional<std::string> port;

    std::filesystem::path configFilePath(configFileName);
    if (!std::filesystem::is_regular_file(configFilePath)) {
        std::cerr << std::format("Error opening configuration file: {}\n", configFileName);
        logEvent(ERROR, "Error opening configuration file: " + configFileName);
        return 1;
    }

    std::ifstream configFile(configFileName);
    std::string line;
    while (std::getline(configFile, line)) {
        std::string_view lineView(line);
        if (lineView.substr(0, 5) == "PORT=") {
            port = lineView.substr(5);
            logEvent(DEBUG, "Port set to " + *port);
            break;
        }
    }
    configFile.close();

    if (!port.has_value()) {
        std::cerr << "Port number not found in configuration file!\n";
        logEvent(ERROR, "Port number not found in configuration file!");
        return 1;
    }

    if ((rv = getaddrinfo(nullptr, port->c_str(), &hints, &servinfo)) != 0) {
        std::cerr << std::format("getaddrinfo: {}\n", gai_strerror(rv));
        logEvent(ERROR, std::string("getaddrinfo: ") + gai_strerror(rv));
        return 1;
    }

    // Loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            std::perror("server: socket");
            logEvent(ERROR, "server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            logEvent(ERROR, "setsockopt");
            throw std::system_error(errno, std::generic_category(), "setsockopt");
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            std::perror("server: bind");
            logEvent(ERROR, "server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        std::cerr << "server: failed to bind\n";
        logEvent(ERROR, "server: failed to bind");
        return 2;
    }

    logEvent(INFO, "server (UDP): waiting for datagrams...");
    std::cout << "server (UDP): waiting for datagrams...\n";

    // UDP receive loop
    while (true) {
        std::array<char, MAXDATASIZE> buf;
        addr_len = sizeof their_addr;
        ssize_t numbytes = recvfrom(sockfd, buf.data(), MAXDATASIZE - 1, 0,
                                    (struct sockaddr*)&their_addr, &addr_len);
        if (numbytes == -1) {
            logEvent(ERROR, "recvfrom");
            std::perror("recvfrom");
            continue;
        }

        buf[numbytes] = '\0';
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
        std::string receivedMsg(buf.data());
        logEvent(INFO, "Datagram from: " + std::string(s));
        logEvent(DEBUG, "Received from " + std::string(s) + ": " + receivedMsg);

        // Example echo response (optional). Adjust / remove as needed.
        std::string reply = "ACK: " + receivedMsg;
        ssize_t sent = sendto(sockfd, reply.data(), reply.size(), 0,
                              (struct sockaddr*)&their_addr, addr_len);
        if (sent == -1) {
            logEvent(ERROR, "sendto");
            std::perror("sendto");
        } else {
            logEvent(DEBUG, "Sent reply to " + std::string(s));
        }
    }

    close(sockfd);
    return 0;
}
// ...existing code...
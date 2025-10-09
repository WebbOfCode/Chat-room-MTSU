#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

enum class Mode { SENDING, RECEIVING, DONE };

static bool sendAll(int fd, const char* buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, buf + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("send");
            return false;
        }
        if (n == 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

static bool sendLine(int fd, const std::string& s) {
    std::string line = s;
    if (line.empty() || line.back() != '\n') line.push_back('\n');
    bool ok = sendAll(fd, line.c_str(), line.size());
    ::fsync(fd); // ensure flush
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // tiny delay for iPad shells
    return ok;
}

static bool recvLine(int fd, std::string& out) {
    out.clear();
    char ch;
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false; // peer closed
        if (ch == '\n') break;
        out.push_back(ch);
        if (out.size() > 65536) return false;
    }
    return true;
}

static int connectTo(const std::string& ip, uint16_t port) {
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "Invalid IPv4 address: " << ip << "\n";
        ::close(s);
        return -1;
    }

    std::cout << "[user2] Connecting to " << ip << ":" << port << " ...\n";
    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect");
        ::close(s);
        return -1;
    }
    std::cout << "[user2] Connected.\n";
    return s;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./user2 <user1_ip> <port>\n";
        return 1;
    }
    std::string ip = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    int sock = connectTo(ip, port);
    if (sock < 0) return 2;

    Mode mode = Mode::SENDING;
    std::cout << "[user2] You will SEND first. Each line is a message.\n"
              << "[user2] Type '#' alone on a line to end your turn. Type 'Exit' to quit.\n";

    while (true) {
        if (mode == Mode::SENDING) {
            std::string my;
            if (!std::getline(std::cin, my)) {
                sendLine(sock, "Exit");
                break;
            }
            if (my == "Exit") {
                sendLine(sock, "Exit");
                std::cout << "[user2] Sent Exit — closing.\n";
                break;
            } else if (my == "#") {
                sendLine(sock, "#");
                std::cout << "[user2] Turn ended. Waiting for peer…\n";
                mode = Mode::RECEIVING;
            } else {
                if (!sendLine(sock, my)) {
                    std::cout << "[user2] Send failed.\n";
                    break;
                } else {
                    std::cout << "[user2] Sent: " << my << "\n";
                }
            }
        } else {
            std::string line;
            if (!recvLine(sock, line)) {
                std::cout << "[user2] Connection closed.\n";
                break;
            }
            if (line == "Exit") {
                std::cout << "[peer] Exit\n";
                sendLine(sock, "Exit");
                break;
            } else if (line == "#") {
                std::cout << "[peer] #  (Your turn to send.)\n";
                mode = Mode::SENDING;
            } else {
                std::cout << "[peer] " << line << "\n";
            }
        }
    }

    ::close(sock);
    std::cout << "[user2] Session ended.\n";
    return 0;
}

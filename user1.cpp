// user1.cpp — Listens on a port, accepts exactly one connection, then:
// 1) RECEIVE until it gets "#"   → then you talk
// 2) SEND from stdin until "#"   → then peer talks
// Either side may send "Exit" (exact line) to end; the other side replies "Exit" and both close.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

enum class Mode { SENDING, RECEIVING, DONE };

static bool sendAll(int fd, const char* buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, buf + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
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
    return sendAll(fd, line.c_str(), line.size());
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
        if (n == 0) {
            // peer closed
            return false;
        }
        if (ch == '\n') break;
        out.push_back(ch);
        // (Optional) guard against absurdly long lines
        if (out.size() > 65536) return false;
    }
    return true;
}

static int listenOnce(uint16_t port) {
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }

    int yes = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt"); ::close(s); return -1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces
    addr.sin_port = htons(port);

    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); ::close(s); return -1;
    }
    if (listen(s, 1) < 0) {
        perror("listen"); ::close(s); return -1;
    }

    std::cout << "[user1] Listening on port " << port << " ...\n[user1] Waiting for user2 to connect...\n";

    sockaddr_in peer{};
    socklen_t plen = sizeof(peer);
    int c = ::accept(s, reinterpret_cast<sockaddr*>(&peer), &plen);
    if (c < 0) { perror("accept"); ::close(s); return -1; }

    char ipbuf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipbuf, sizeof(ipbuf));
    std::cout << "[user1] Connected to " << ipbuf << ":" << ntohs(peer.sin_port) << "\n";

    ::close(s); // no more accepts; single peer only
    return c;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./user1 <port>\n";
        return 1;
   }
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
    int sock = listenOnce(port);
    if (sock < 0) return 2;

    Mode mode = Mode::RECEIVING;  // per spec, user2 talks first
    bool done = false;
    bool sentExit = false;

    std::cout << "[user1] You will RECEIVE first. Peer will type until '#'.\n";

    while (!done) {
        if (mode == Mode::RECEIVING) {
            std::string line;
            if (!recvLine(sock, line)) {
                std::cout << "[user1] Connection closed or error.\n";
                break;
            }
            if (line == "Exit") {
                std::cout << "[peer] Exit\n";
                // Reply Exit and end
                sendLine(sock, "Exit");
                break;
            } else if (line == "#") {
                std::cout << "[peer] #  (Your turn to send. Type lines; end your turn with '#'. Type 'Exit' to finish.)\n";
                mode = Mode::SENDING;
            } else {
                std::cout << "[peer] " << line << "\n";
            }
        } else if (mode == Mode::SENDING) {
            std::string my;
            if (!std::getline(std::cin, my)) {
                // EOF on stdin → be polite and exit
                sendLine(sock, "Exit");
                break;
            }
            if (my == "Exit") {
                sendLine(sock, "Exit");
                sentExit = true;
                // Wait for peer Exit echo then close (or peer may close immediately)
                std::string echo;
                if (recvLine(sock, echo) && echo == "Exit") {
                    std::cout << "[peer] Exit\n";
                }
                break;
            } else if (my == "#") {
                sendLine(sock, "#");
                std::cout << "[user1] Turn ended. Waiting for peer…\n";
                mode = Mode::RECEIVING;
            } else {
                if (!sendLine(sock, my)) {
                    std::cout << "[user1] Send failed.\n";
                    break;
                }
            }
        } else {
            done = true;
        }
    }

    ::close(sock);
    std::cout << "[user1] Session ended.\n";
    return 0;
}

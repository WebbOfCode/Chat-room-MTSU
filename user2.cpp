#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
// Removed <thread> (unused)

using namespace std; // Requested global using for simplicity

enum class Mode { SENDING, RECEIVING, DONE }; // same phases as server

// sendAll: loop until buffer fully sent or error
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

// sendLine: append newline if missing
static bool sendLine(int fd, const string& s) {
    string line = s;
    if (line.empty() || line.back() != '\n') line.push_back('\n');
    bool ok = sendAll(fd, line.c_str(), line.size());
    return ok;
}

// recvLine: read until newline; false on error/closure
static bool recvLine(int fd, string& out) {
    out.clear();
    char ch;
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            return false;
        }
        if (n == 0) return false; // peer closed
        if (ch == '\n') break;
        out.push_back(ch);
        if (out.size() > 65536) return false;
    }
    return true;
}

// connectTo: create socket and connect to given ip:port
static int connectTo(const string& ip, uint16_t port) {
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) { perror("socket"); return -1; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        cerr << "Invalid IPv4 address: " << ip << "\n";
        ::close(s);
        return -1;
    }

    cout << "[user2] Connecting to " << ip << ":" << port << " ...\n";
    if (connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect");
        ::close(s);
        return -1;
    }
    // Disable Nagle to reduce latency for small chat messages
    int one = 1;
    if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) < 0) {
        perror("setsockopt(TCP_NODELAY)");
        // non-fatal
    }

    cout << "[user2] Connected.\n";
    return s;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Usage: ./user2 <user1_ip> <port>\n";
        return 1;
    }
    string ip = argv[1];
    uint16_t port = static_cast<uint16_t>(stoi(argv[2]));

    int sock = connectTo(ip, port);
    if (sock < 0) return 2;

    Mode mode = Mode::SENDING;
    cout << "[user2] SEND first. End turn with '#'. Type 'Exit' to quit.\n";

    while (true) {
        if (mode == Mode::SENDING) {
            string my;
            if (!getline(cin, my)) {
                sendLine(sock, "Exit");
                break;
            }
            if (my == "Exit") {
                sendLine(sock, "Exit");
                cout << "[user2] Sent Exit — closing.\n";
                break;
            } else if (my == "#") {
                sendLine(sock, "#");
                cout << "[user2] Turn ended. Waiting for peer…\n";
                mode = Mode::RECEIVING;
            } else {
                if (!sendLine(sock, my)) {
                    cout << "[user2] Send failed.\n";
                    break;
                } else {
                    cout << "[user2] Sent: " << my << "\n";
                }
            }
        } else {
            string line;
            if (!recvLine(sock, line)) {
                cout << "[user2] Connection closed.\n";
                break;
            }
            if (line == "Exit") {
                cout << "[peer] Exit\n";
                sendLine(sock, "Exit");
                break;
            } else if (line == "#") {
                cout << "[peer] #  (Your turn)\n";
                mode = Mode::SENDING;
            } else {
                cout << "[peer] " << line << "\n";
            }
        }
    }

    ::close(sock);
    cout << "[user2] Session ended.\n";
    return 0;
}

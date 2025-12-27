#pragma once
#include "interfaces.hpp"
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdexcept>
#include <iostream>

class RealNetwork : public INetwork {
    int sock_fd = -1;
    const int TIMEOUT_SEC = 5;

public:
    ~RealNetwork() { disconnect(); }

    void disconnect() override {
        if (sock_fd >= 0) {
            close(sock_fd);
            sock_fd = -1;
        }
    }

    void connect(const std::string& host, int port) override {
        disconnect();

        struct addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) {
            throw std::runtime_error("DNS Error");
        }

        sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock_fd < 0) {
            freeaddrinfo(res);
            throw std::runtime_error("Socket Creation Error");
        }

        // ВАЖНО: Таймауты на "висящий" кабель
        struct timeval tv;
        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;
        setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        if (::connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
            freeaddrinfo(res);
            close(sock_fd);
            sock_fd = -1;
            throw std::runtime_error("Connect Failed");
        }
        freeaddrinfo(res);
    }

    void send_all(const std::string& data) override {
        if (sock_fd < 0) throw std::runtime_error("No Connection");
        size_t sent = 0;
        while (sent < data.size()) {
            ssize_t res = send(sock_fd, data.c_str() + sent, data.size() - sent, 0); // Mac: MSG_NOSIGNAL через setsockopt или игнор SIGPIPE в main
            if (res <= 0) throw std::runtime_error("Send Error");
            sent += res;
        }
    }

    std::vector<uint8_t> recv_exact(size_t size) override {
        if (sock_fd < 0) throw std::runtime_error("No Connection");
        std::vector<uint8_t> buffer(size);
        size_t received = 0;
        while (received < size) {
            ssize_t res = recv(sock_fd, buffer.data() + received, size - received, 0);
            if (res <= 0) throw std::runtime_error("Recv Timeout/Closed");
            received += res;
        }
        return buffer;
    }

    std::string recv_until(const std::string& target) override {
        if (sock_fd < 0) throw std::runtime_error("No Connection");
        std::string buf;
        char temp[1];
        while (true) {
            ssize_t res = recv(sock_fd, temp, 1, 0);
            if (res <= 0) throw std::runtime_error("Recv Timeout/Closed");
            buf += temp[0];
            if (buf.find(target) != std::string::npos) return buf;
            if (buf.size() > 1024) throw std::runtime_error("Handshake Overflow");
        }
    }
};
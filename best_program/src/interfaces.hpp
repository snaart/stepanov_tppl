#pragma once
#include <string>
#include <vector>
#include <cstdint>

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual void write_line(const std::string& line) = 0;
};

class INetwork {
public:
    virtual ~INetwork() = default;
    virtual void connect(const std::string& host, int port) = 0;
    virtual void disconnect() = 0;
    virtual void send_all(const std::string& data) = 0;
    virtual std::vector<uint8_t> recv_exact(size_t size) = 0;
    virtual std::string recv_until(const std::string& target) = 0;
};
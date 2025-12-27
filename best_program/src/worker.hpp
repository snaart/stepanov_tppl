#pragma once
#include "interfaces.hpp"
#include "protocol.hpp"
#include <thread>
#include <iostream>
#include <memory>

class DataWorker {
    std::shared_ptr<INetwork> net;
    std::shared_ptr<IFileSystem> fs;
    std::string host;
    int port;
    int type;
    volatile bool running = true;

public:
    DataWorker(std::shared_ptr<INetwork> n, std::shared_ptr<IFileSystem> f,
               std::string h, int p, int t)
        : net(n), fs(f), host(h), port(p), type(t) {}

    void stop() { running = false; }

    void run_loop() {
        while (running) {
            try {
                std::cout << "[Port " << port << "] Connecting..." << std::endl;
                net->connect(host, port);

                // Handshake
                net->send_all("isu_pt");
                std::string resp = net->recv_until("granted");
                std::cout << "[Port " << port << "] Auth Success." << std::endl;

                // Data Loop
                size_t expected = (type == 1) ? 15 : 21;
                while (running) {
                    net->send_all("get");
                    auto buffer = net->recv_exact(expected);

                    uint8_t recv_sum = buffer.back();
                    uint8_t calc_sum = Protocol::calculate_checksum(buffer);

                    if (recv_sum != calc_sum) {
                        std::cerr << "[Port " << port << "] Checksum fail!" << std::endl;
                        continue;
                    }

                    // Parse
                    int offset = 0;
                    int64_t ts = Protocol::parse_int64(buffer.data()); offset += 8;
                    std::string log = Protocol::format_time(ts) + " ";

                    if (type == 1) {
                        float temp = Protocol::parse_float(buffer.data() + offset); offset += 4;
                        int16_t press = Protocol::parse_int16(buffer.data() + offset);
                        std::stringstream ss;
                        ss << log << "Server1 Temp: " << std::fixed << std::setprecision(2) << temp << ", Pressure: " << press;
                        log = ss.str();
                    } else {
                        int32_t x = Protocol::parse_int32(buffer.data() + offset); offset += 4;
                        int32_t y = Protocol::parse_int32(buffer.data() + offset); offset += 4;
                        int32_t z = Protocol::parse_int32(buffer.data() + offset);
                        log += "Server2 X: " + std::to_string(x) + ", Y: " + std::to_string(y) + ", Z: " + std::to_string(z);
                    }

                    fs->write_line(log);
                }

            } catch (const std::exception& e) {
                std::cerr << "[Port " << port << "] Error: " << e.what() << ". Retry in 2s..." << std::endl;
                net->disconnect();
            }

            if (running) std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
};
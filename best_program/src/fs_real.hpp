#pragma once
#include "interfaces.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <mutex>

class RealFileSystem : public IFileSystem {
    int fd;
    std::mutex mtx;
public:
    RealFileSystem(const std::string& filename) {
        // O_DSYNC/O_SYNC может отсутствовать на Mac, используем fsync вручную
        fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) {
            std::cerr << "CRITICAL: Cannot open " << filename << std::endl;
            exit(1);
        }
    }

    ~RealFileSystem() {
        if (fd >= 0) close(fd);
    }

    void write_line(const std::string& line) override {
        std::lock_guard<std::mutex> lock(mtx);
        std::string payload = line + "\n";

        ssize_t written = 0;
        size_t total = payload.size();
        const char* ptr = payload.c_str();

        while (written < total) {
            ssize_t res = write(fd, ptr + written, total - written);
            if (res < 0) {
                if (errno == EINTR) continue;
                std::cerr << "Disk Error!" << std::endl;
                return;
            }
            written += res;
        }

        // --- ГАРАНТИЯ СОХРАНЕНИЯ ПРИ ВЫКЛЮЧЕНИИ ПИТАНИЯ ---
        fsync(fd);

        std::cout << line << std::endl;
    }
};
#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Protocol {
    inline uint8_t calculate_checksum(const std::vector<uint8_t>& data) {
        int sum = 0;
        for (size_t i = 0; i < data.size() - 1; ++i) sum = (sum + data[i]) % 256;
        return static_cast<uint8_t>(sum);
    }

    inline int64_t parse_int64(const uint8_t* ptr) {
        uint64_t val = 0;
        for(int i=0; i<8; ++i) val = (val << 8) | ptr[i];
        return static_cast<int64_t>(val);
    }

    inline int32_t parse_int32(const uint8_t* ptr) {
        uint32_t val = 0;
        for(int i=0; i<4; ++i) val = (val << 8) | ptr[i];
        return static_cast<int32_t>(val);
    }

    inline int16_t parse_int16(const uint8_t* ptr) {
        uint16_t val = 0;
        for(int i=0; i<2; ++i) val = (val << 8) | ptr[i];
        return static_cast<int16_t>(val);
    }

    inline float parse_float(const uint8_t* ptr) {
        uint32_t temp = 0;
        for(int i=0; i<4; ++i) temp = (temp << 8) | ptr[i];
        float result;
        std::memcpy(&result, &temp, sizeof(result));
        return result;
    }

    inline std::string format_time(int64_t micros) {
        time_t seconds = micros / 1000000;
        struct tm* tm_info = std::gmtime(&seconds);
        char buffer[30];
        strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", tm_info);
        return std::string(buffer);
    }
}
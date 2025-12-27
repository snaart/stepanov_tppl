#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <cstring>
#include <csignal>
#include <cmath>
#include <iomanip>

// POSIX System Calls
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>

// Конфигурация
const std::string HOST = "95.163.237.76";
const int PORT_1 = 5123;
const int PORT_2 = 5124;
const std::string OUTPUT_FILE = "data.txt";
const int PACKET_SIZE_1 = 15;
const int PACKET_SIZE_2 = 21;
const int RECONNECT_DELAY_SEC = 2;
const int SOCKET_TIMEOUT_SEC = 5;

// Глобальные флаги для управления жизненным циклом
std::atomic<bool> running{true};
std::mutex log_mutex;

// ----------------------------------------------------------------------
// Класс для надежной записи на диск (Nuclear-proof)
// ----------------------------------------------------------------------
// Добавляем необходимые хедеры для определения платформы
#if defined(_WIN32) || defined(_WIN64)
    #include <io.h>      // Для _write, _commit, _open
    #include <fcntl.h>
    // Windows-макросы для флагов открытия файла
    #define MY_OPEN_FLAGS (O_WRONLY | O_CREAT | O_APPEND | O_BINARY)
    #define MY_SYNC_FUNC(fd) _commit(fd)
#else
    #include <unistd.h>  // Для write, fsync, close
    #include <fcntl.h>
    // Linux/macOS макросы. O_DSYNC - это "строгий" режим записи
    // Если O_DSYNC не определен (например, на macOS), используем 0
    #ifndef O_DSYNC
        #define O_DSYNC 0
    #endif
    #define MY_OPEN_FLAGS (O_WRONLY | O_CREAT | O_APPEND | O_DSYNC)
    #define MY_SYNC_FUNC(fd) fsync(fd)
#endif

class SafeLogger {
    int fd;
public:
    SafeLogger(const std::string& filename) {
        // Открываем файл с правами 0644 (rw-r--r--)
        #if defined(_WIN32) || defined(_WIN64)
             // _sopen_s - безопасный аналог open на Windows
             errno_t err = _sopen_s(&fd, filename.c_str(), MY_OPEN_FLAGS, _SH_DENYNO, _S_IREAD | _S_IWRITE);
             if (err != 0) fd = -1;
        #else
             fd = open(filename.c_str(), MY_OPEN_FLAGS, 0644);
        #endif

        if (fd < 0) {
            std::cerr << "CRITICAL ERROR: Cannot open file " << filename << std::endl;
            exit(1);
        }
    }

    ~SafeLogger() {
        if (fd >= 0) {
            #if defined(_WIN32) || defined(_WIN64)
                _close(fd);
            #else
                close(fd);
            #endif
        }
    }

    void write_line(const std::string& line) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::string payload = line + "\n";

        ssize_t written = 0;
        size_t total = payload.size();
        const char* ptr = payload.c_str();

        while (written < total) {
            #if defined(_WIN32) || defined(_WIN64)
                int res = _write(fd, ptr + written, (unsigned int)(total - written));
            #else
                ssize_t res = write(fd, ptr + written, total - written);
            #endif

            if (res < 0) {
                // Если прервано сигналом (EINTR), пробуем снова. Иначе ошибка.
                if (errno == EINTR) continue;
                std::cerr << "Disk Write Error: " << strerror(errno) << std::endl;
                return;
            }
            written += res;
        }

        // --- САМОЕ ВАЖНОЕ: ПРИНУДИТЕЛЬНЫЙ СБРОС НА ДИСК ---
        // Гарантирует, что данные физически записаны
        MY_SYNC_FUNC(fd);

        std::cout << line << std::endl;
    }
};
// Глобальный логгер
SafeLogger logger(OUTPUT_FILE);

// ----------------------------------------------------------------------
// Утилиты парсинга (Big Endian -> Host)
// ----------------------------------------------------------------------
namespace Protocol {
    uint8_t calculate_checksum(const std::vector<uint8_t>& data) {
        int sum = 0;
        // Считаем сумму всех байт кроме последнего (который сам checksum)
        for (size_t i = 0; i < data.size() - 1; ++i) {
            sum = (sum + data[i]) % 256;
        }
        return static_cast<uint8_t>(sum);
    }

    int64_t parse_int64(const uint8_t* ptr) {
        // Ручная сборка big-endian int64 для безопасности
        uint64_t val = 0;
        for(int i=0; i<8; ++i) val = (val << 8) | ptr[i];
        return static_cast<int64_t>(val);
    }

    int32_t parse_int32(const uint8_t* ptr) {
        uint32_t val = 0;
        for(int i=0; i<4; ++i) val = (val << 8) | ptr[i];
        return static_cast<int32_t>(val);
    }

    int16_t parse_int16(const uint8_t* ptr) {
        uint16_t val = 0;
        for(int i=0; i<2; ++i) val = (val << 8) | ptr[i];
        return static_cast<int16_t>(val);
    }

    float parse_float(const uint8_t* ptr) {
        // Lua использует IEEE 754. Читаем как uint32 big-endian, затем memcpy в float
        uint32_t temp = 0;
        for(int i=0; i<4; ++i) temp = (temp << 8) | ptr[i];
        float result;
        std::memcpy(&result, &temp, sizeof(result));
        return result;
    }

    std::string format_time(int64_t micros) {
        time_t seconds = micros / 1000000;
        struct tm* tm_info = std::gmtime(&seconds); // Используем UTC как в Lua скрипте (!date)
        char buffer[30];
        strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", tm_info);
        return std::string(buffer);
    }
}

// ----------------------------------------------------------------------
// Класс TCP Клиента
// ----------------------------------------------------------------------
class TcpWorker {
    std::string host;
    int port;
    int type; // 1 или 2
    int sock_fd;

public:
    TcpWorker(std::string h, int p, int t) : host(h), port(p), type(t), sock_fd(-1) {}

    // Основной цикл рабочего потока
    void run() {
        while (running) {
            try {
                connect_to_server();
                handshake();
                data_loop();
            } catch (const std::exception& e) {
                std::cerr << "[Port " << port << "] Error: " << e.what() << ". Reconnecting..." << std::endl;
            } catch (...) {
                std::cerr << "[Port " << port << "] Unknown error. Reconnecting..." << std::endl;
            }

            close_socket();
            if (running) std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_DELAY_SEC));
        }
    }

private:
    void close_socket() {
        if (sock_fd >= 0) {
            close(sock_fd);
            sock_fd = -1;
        }
    }

    void connect_to_server() {
        close_socket();

        struct addrinfo hints{}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        std::string port_str = std::to_string(port);
        if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
            throw std::runtime_error("DNS resolution failed");
        }

        sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock_fd < 0) {
            freeaddrinfo(res);
            throw std::runtime_error("Socket creation failed");
        }

        // Установка таймаутов (SO_RCVTIMEO, SO_SNDTIMEO)
        struct timeval tv;
        tv.tv_sec = SOCKET_TIMEOUT_SEC;
        tv.tv_usec = 0;
        setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
        setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

        std::cout << "[Port " << port << "] Connecting..." << std::endl;
        if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
            freeaddrinfo(res);
            throw std::runtime_error("Connect failed");
        }
        freeaddrinfo(res);
        std::cout << "[Port " << port << "] Connected." << std::endl;
    }

    void send_all(const std::string& data) {
        size_t sent = 0;
        while (sent < data.size()) {
            ssize_t res = send(sock_fd, data.c_str() + sent, data.size() - sent, MSG_NOSIGNAL);
            if (res <= 0) throw std::runtime_error("Send failed");
            sent += res;
        }
    }

    // Чтение точного количества байт
    void recv_exact(std::vector<uint8_t>& buffer, size_t size) {
        buffer.resize(size);
        size_t received = 0;
        while (received < size) {
            ssize_t res = recv(sock_fd, buffer.data() + received, size - received, 0);
            if (res <= 0) {
                 throw std::runtime_error("Connection closed or timed out");
            }
            received += res;
        }
    }

    // Чтение до нахождения подстроки (для авторизации)
    std::string recv_until(const std::string& target) {
        std::string buf;
        char temp[1];
        while (running) {
            ssize_t res = recv(sock_fd, temp, 1, 0);
            if (res <= 0) throw std::runtime_error("Recv failed during handshake");
            buf += temp[0];
            if (buf.find(target) != std::string::npos) return buf;
            if (buf.size() > 1024) throw std::runtime_error("Handshake buffer overflow");
        }
        return "";
    }

    void handshake() {
        send_all("isu_pt");
        std::string resp = recv_until("granted");
        std::cout << "[Port " << port << "] Auth Success." << std::endl;
    }

    void data_loop() {
        size_t expected_size = (type == 1) ? PACKET_SIZE_1 : PACKET_SIZE_2;
        std::vector<uint8_t> buffer;

        while (running) {
            // 1. Отправляем запрос
            send_all("get");

            // 2. Читаем ответ
            recv_exact(buffer, expected_size);

            // 3. Проверка контрольной суммы
            uint8_t received_checksum = buffer.back();
            uint8_t calc_checksum = Protocol::calculate_checksum(buffer);

            if (received_checksum != calc_checksum) {
                std::cerr << "[Port " << port << "] Checksum mismatch! Ignoring packet." << std::endl;
                continue;
            }

            // 4. Парсинг
            int offset = 0;
            int64_t timestamp = Protocol::parse_int64(buffer.data() + offset); offset += 8;
            std::string date_str = Protocol::format_time(timestamp);

            std::ostringstream ss;
            ss << date_str << " ";

            if (type == 1) {
                float temp = Protocol::parse_float(buffer.data() + offset); offset += 4;
                int16_t pressure = Protocol::parse_int16(buffer.data() + offset); offset += 2;
                ss << "Server1 Temp: " << std::fixed << std::setprecision(2) << temp
                   << ", Pressure: " << pressure;
            } else {
                int32_t x = Protocol::parse_int32(buffer.data() + offset); offset += 4;
                int32_t y = Protocol::parse_int32(buffer.data() + offset); offset += 4;
                int32_t z = Protocol::parse_int32(buffer.data() + offset); offset += 4;
                ss << "Server2 X: " << x << ", Y: " << y << ", Z: " << z;
            }

            // 5. Надежная запись
            logger.write_line(ss.str());
        }
    }
};

// ----------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------
void signal_handler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Stopping..." << std::endl;
    running = false;
}

int main() {
    // Игнорируем SIGPIPE, чтобы программа не падала при записи в разорванный сокет
    signal(SIGPIPE, SIG_IGN);
    // Обрабатываем Ctrl+C и Termination
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "Starting reliable data collector..." << std::endl;

    TcpWorker w1(HOST, PORT_1, 1);
    TcpWorker w2(HOST, PORT_2, 2);

    std::thread t1(&TcpWorker::run, &w1);
    std::thread t2(&TcpWorker::run, &w2);

    t1.join();
    t2.join();

    std::cout << "Program stopped gracefully." << std::endl;
    return 0;
}
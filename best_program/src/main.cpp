#include "worker.hpp"
#include "network_real.hpp"
#include "fs_real.hpp"
#include <csignal>

// Глобальные указатели для управления
std::shared_ptr<DataWorker> w1, w2;

void signal_handler(int) {
    if(w1) w1->stop();
    if(w2) w2->stop();
}

int main() {
    // Игнорируем SIGPIPE, чтобы не падать при записи в мертвый сокет
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Абсолютный путь, чтобы найти файл!
    // Замените на свой путь, если нужно
    std::string path = "data.txt";

    // Единый логгер (потокобезопасный внутри)
    auto fs = std::make_shared<RealFileSystem>(path);

    // Создаем воркеров
    auto net1 = std::make_shared<RealNetwork>();
    w1 = std::make_shared<DataWorker>(net1, fs, "95.163.237.76", 5123, 1);

    auto net2 = std::make_shared<RealNetwork>();
    w2 = std::make_shared<DataWorker>(net2, fs, "95.163.237.76", 5124, 2);

    std::thread t1([&](){ w1->run_loop(); });
    std::thread t2([&](){ w2->run_loop(); });

    t1.join();
    t2.join();

    std::cout << "Exited." << std::endl;
    return 0;
}
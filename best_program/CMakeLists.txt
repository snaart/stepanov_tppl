cmake_minimum_required(VERSION 3.14)
project(ReliableDataCollector)

# C++26 экспериментальный, лучше использовать 17 или 20 для стабильности GTest
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(collector
        src/main.cpp
        src/worker.hpp
        src/protocol.hpp
        src/interfaces.hpp
        src/network_real.hpp
        src/fs_real.hpp
)

# Подключение потоков (нужно для Linux/Mac)
find_package(Threads REQUIRED)
target_link_libraries(collector Threads::Threads)

# --- Тесты (Google Test) ---
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
        # Исправляет Warning CMP0135
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Для Windows (на всякий случай оставляем)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

include_directories(include)

enable_testing()

add_executable(unit_tests
        tests/unit_tests.cpp
        src/worker.hpp
        src/protocol.hpp
        src/interfaces.hpp
)

# ИСПРАВЛЕНИЕ ЗДЕСЬ:
# Добавляем GTest::gmock, чтобы видеть <gmock/gmock.h>
target_link_libraries(unit_tests
        GTest::gtest_main
        GTest::gmock
)

# Команда для удобного запуска тестов
add_custom_target(run_tests COMMAND unit_tests)
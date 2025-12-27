cmake_minimum_required(VERSION 3.14)
project(ReliableDataCollector)

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

find_package(Threads REQUIRED)
target_link_libraries(collector Threads::Threads)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

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

target_link_libraries(unit_tests
        GTest::gtest_main
        GTest::gmock
)

add_custom_target(run_tests COMMAND unit_tests)
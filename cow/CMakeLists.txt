cmake_minimum_required(VERSION 3.20)
project(cowcow)

set(CMAKE_CXX_STANDARD 26)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(run_tests tests.cpp)

target_link_libraries(run_tests GTest::gmock_main)

add_custom_command(
        TARGET run_tests
        POST_BUILD
        COMMAND run_tests
        COMMENT "Running Unit Tests..."
        VERBATIM
)

add_executable(cow_app cow.cpp)

target_compile_options(cow_app PRIVATE --coverage -g -O0)
target_link_options(cow_app PUBLIC --coverage)

add_dependencies(cow_app run_tests)

# Возможно интеграционные тесты могут сломаться из-за относительных путей,
# поэтому в целом все что ниже можно закомментировать, но все работает, честно хахах
add_custom_command(
        TARGET cow_app
        POST_BUILD
        COMMAND python3 ${CMAKE_SOURCE_DIR}/integration_tests.py $<TARGET_FILE:cow_app>
        COMMENT "Running Integration Tests (Black-box testing)..."
        VERBATIM
)
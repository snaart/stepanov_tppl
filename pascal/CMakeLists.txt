cmake_minimum_required(VERSION 3.28)
project(pascal)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(include)

add_executable(pascal src/main.cpp src/AppConfig.cpp src/Lexer.cpp src/Parser.cpp src/Interpreter.cpp src/SemanticAnalyzer.cpp)

add_executable(test_pascal    tests/test_main.cpp
    tests/test_types.cpp
 src/AppConfig.cpp src/Lexer.cpp src/Parser.cpp src/Interpreter.cpp src/SemanticAnalyzer.cpp)
target_link_libraries(test_pascal gtest_main)

add_executable(test_integration tests/test_integration.cpp src/AppConfig.cpp src/Lexer.cpp src/Parser.cpp src/Interpreter.cpp src/SemanticAnalyzer.cpp)
target_link_libraries(test_integration gtest_main)

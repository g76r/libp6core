cmake_minimum_required(VERSION 3.18)
project(test LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Test)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
enable_testing(true)

include_directories(../../)
link_directories( ../../../build-p6core-linux/release)

add_executable(mathexpr tst_mathexpr.cpp)
add_test(NAME mathexpr COMMAND mathexpr)
target_link_libraries(mathexpr PRIVATE p6core Qt::Test)

cmake_minimum_required(VERSION 3.10)

project(evtsigslot)
set(CMAKE_CXX_STANDARD 17)

add_executable(test test/signal.cpp)
add_executable(test1 test/signal-test.cpp)
add_executable(test-performance test/signal-performance.cpp)

include_directories(include)


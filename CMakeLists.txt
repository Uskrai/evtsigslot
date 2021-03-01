cmake_minimum_required(VERSION 3.10)

project(evtsigslot)
set(CMAKE_CXX_STANDARD 17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(test test/signal.cpp)
add_executable(signal test/signal-test.cpp)
add_executable(performance test/signal-performance.cpp)
add_executable(thread test/signal-thread.cpp)

target_link_libraries(thread PRIVATE Threads::Threads)

include_directories(include)


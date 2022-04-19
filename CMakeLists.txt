cmake_minimum_required(VERSION 3.5)
project(VectorSilKitQemuDemos)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE=1)
target_link_libraries(asio INTERFACE Threads::Threads)

add_subdirectory(demos/guest)
add_subdirectory(demos/host)


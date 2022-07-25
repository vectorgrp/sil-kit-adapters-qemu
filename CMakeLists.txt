cmake_minimum_required(VERSION 3.5)
project(VectorSilKitQemuDemos)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

option(BUILD_QEMU_TIMESYNC_PLUGIN "Build the QEMU Time-Sync. Plugin" OFF)

find_package(Threads REQUIRED)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE=1)
target_link_libraries(asio INTERFACE Threads::Threads)
if (WIN32)
    target_compile_definitions(asio INTERFACE _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING=1)
    target_compile_definitions(asio INTERFACE WINVER=0x0601 _WIN32_WINNT=0x0601) # Windows 7
endif()

if(NOT IB_DIR)
    message(FATAL_ERROR "Please set the PATH to the Integration Bus in IB_DIR: e.g."
        " cmake -D IB_DIR=../IntegrationBus-x.y.z-Platform/")
endif()
message(STATUS "Searching IntegrationBus package in ${IB_DIR}")
find_package(IntegrationBus 3.7.14
    REQUIRED
    CONFIG
    NO_CMAKE_PACKAGE_REGISTRY
    NO_DEFAULT_PATH
    PATHS "${IB_DIR}"
)
if(NOT TARGET IntegrationBus::IntegrationBus)
    message(FATAL_ERROR "Could not find Integration Bus package in ${IB_DIR}")
endif()

set(IB_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(demos/Ethernet)

if(BUILD_QEMU_TIMESYNC_PLUGIN)
    message(NOTICE "-- ${PROJECT_NAME}: Building the QEMU Time-Sync. Plugin")
    add_subdirectory(demos/TimeSyncPlugin)
endif()

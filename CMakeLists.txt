cmake_minimum_required(VERSION 3.5)
project(VectorSilKitAdaptersQemu)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

set(SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

option(BUILD_QEMU_CHARDEV_ADAPTER "Build the QEMU Chardev Adapter and Demos" OFF)

find_package(Threads REQUIRED)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE=1)
target_link_libraries(asio INTERFACE Threads::Threads)

if (WIN32)
    target_compile_definitions(asio INTERFACE _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING=1)
    target_compile_definitions(asio INTERFACE WINVER=0x0601 _WIN32_WINNT=0x0601) # Windows 7
endif()

if(NOT SILKIT_PACKAGE_DIR)
    message(FATAL_ERROR "Please set the PATH to the SIL Kit in SILKIT_PACKAGE_DIR: e.g."
        " cmake -D SILKIT_PACKAGE_DIR=../SilKit-x.y.z-Platform/")
endif()
message(STATUS "Searching SilKit package in ${SILKIT_PACKAGE_DIR}")
find_package(SilKit 4.0.7
    REQUIRED
    CONFIG
    NO_CMAKE_PACKAGE_REGISTRY
    NO_DEFAULT_PATH
    PATHS "${SILKIT_PACKAGE_DIR}"
)
if(NOT TARGET SilKit::SilKit)
    message(FATAL_ERROR "Could not find SilKit package in ${SILKIT_PACKAGE_DIR}")
endif()

set(SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(eth/Utility)
add_subdirectory(eth/adapter)
add_subdirectory(eth/demos)

if(BUILD_QEMU_CHARDEV_ADAPTER) 
    message(NOTICE "-- ${PROJECT_NAME}: Building the QEMU Chardev Adapter")
    add_subdirectory(chardev/adapter)
    add_subdirectory(chardev/demos)
endif()

cmake_minimum_required(VERSION 3.5)
project(VectorSilKitAdaptersQemu)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

set(SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

find_package(Threads REQUIRED)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE=1)
target_link_libraries(asio INTERFACE Threads::Threads)

if (WIN32)
    target_compile_definitions(asio INTERFACE _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING=1)
    target_compile_definitions(asio INTERFACE WINVER=0x0601 _WIN32_WINNT=0x0601) # Windows 7
endif()

# SIL KIT PACKAGE CONSUMPTION 

## On Windows, if an msi-installed version of SIL Kit is available and you want to build the adapters against it, CMAKE will attempt to use it.

## If you want to use another specific SIL Kit package (downloaded Release package OR self-built & installed package), you must specify the path to it by setting SILKIT_PACKAGE_DIR
## if it is a downloaded Release package of SIL Kit:
##			"cmake -S. -Bbuild -DSILKIT_PACKAGE_DIR=/path/to/SilKit-x.y.z-Platform/"
## if it is a self-build and installed package of SIL Kit:
##			"cmake -S. -Bbuild -DSILKIT_PACKAGE_DIR=/path/to/self-installed/package/"

if(DEFINED SILKIT_PACKAGE_DIR)
# if the user forces using a specific SIL Kit package, use it
	message(STATUS "SILKIT_PACKAGE_DIR has been set to: ${SILKIT_PACKAGE_DIR}, CMAKE will look for a SIL Kit package in that directory")
	find_package(SilKit 4.0.7
		REQUIRED 
		CONFIG  
		NO_CMAKE_PACKAGE_REGISTRY 
		NO_DEFAULT_PATH 
		PATHS "${SILKIT_PACKAGE_DIR}")
else()
# otherwise, look for an installed version of SIL Kit (.msi file) 
	message(STATUS "SILKIT_PACKAGE_DIR has not been set by user. Attempting to find an msi-installed version of SIL Kit")
	find_package(SilKit 4.0.7
		CONFIG)
endif()

if(SilKit_FOUND)
	message(STATUS "Found a SIL Kit package: [version : ${SilKit_VERSION}]") 
	message(STATUS "[location : ${SilKit_DIR}]")
else()
	# If SilKit is not found, prompt user to specify path
	message (STATUS "No SIL Kit package was found. Make sure you have a compatible SIL Kit version and verify that the SILKIT_PACKAGE_DIR is correctly set in order to find it.")
	message(FATAL_ERROR "Check related comments in CMakeLists.txt file to see how SILKIT_PACKAGE_DIR can be correctly set.")
endif()
	
if(TARGET SilKit::SilKit)
    message(STATUS "SIL Kit package has been successfully imported as a CMake target. [version : ${SilKit_VERSION}]")
else()
	message(FATAL_ERROR "Something went wrong : Could not find SIL Kit package.")
endif()

set(SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

add_subdirectory(adapter)
add_subdirectory(eth/Utility)
add_subdirectory(eth/adapter)
add_subdirectory(eth/demos)

add_subdirectory(chardev/adapter)
add_subdirectory(chardev/demos)

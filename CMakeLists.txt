cmake_minimum_required(VERSION 3.5)
project(VectorSilKitAdaptersQemu)
include(FetchContent)

set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

set(SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

set(SILKIT_VERSION "4.0.43" CACHE STRING "If no SIL Kit package is specified, this version will be downloaded")
set(SILKIT_FLAVOR "ubuntu-18.04-x86_64-gcc" CACHE STRING "If no SIL Kit package is specified, this package flavor will be downloaded")

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
    if (WIN32)
        # otherwise, look for an installed version of SIL Kit (.msi file) 
        message(STATUS "SILKIT_PACKAGE_DIR has not been set by user. Attempting to find an msi-installed version of SIL Kit")
        find_package(SilKit 4.0.7
            CONFIG)
    endif()
        
    if(NOT SilKit_FOUND)
        message(STATUS "No version of SIL Kit present. Attempting to fetch [SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}] from github.com")
        FetchContent_Declare(
                silkit
                URL https://github.com/vectorgrp/sil-kit/releases/download/sil-kit%2Fv${SILKIT_VERSION}/SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}.zip
                DOWNLOAD_DIR ${CMAKE_CURRENT_LIST_DIR}/Downloads                
            )

        message(STATUS "SIL Kit: fetching [SilKit-${SILKIT_VERSION}-${SILKIT_FLAVOR}]")
        FetchContent_MakeAvailable(silkit)

        set(SILKIT_SOURCE_DIR ${silkit_SOURCE_DIR}/SilKit-Source)
        message(STATUS "SIL Kit: using source code from: \"${SILKIT_SOURCE_DIR}\"")
        message(STATUS "SIL Kit: using pre-built binaries from: ${silkit_SOURCE_DIR}")

        message(STATUS "Searching SilKit package which has been fetched from github.com")
        find_package(SilKit 4.0.7
            REQUIRED
            CONFIG
            NO_CMAKE_PACKAGE_REGISTRY
            NO_DEFAULT_PATH
            PATHS "${silkit_SOURCE_DIR}"
        )
    endif()	
endif()

	
if(TARGET SilKit::SilKit)
    message(STATUS "SIL Kit package has been successfully imported as a CMake target. [version : ${SilKit_VERSION}]")
else()
	message(FATAL_ERROR "Something went wrong : Could not find SIL Kit package.")
endif()

add_subdirectory(adapter)
add_subdirectory(eth/Utility)
add_subdirectory(eth/adapter)
add_subdirectory(eth/demos)

add_subdirectory(chardev/adapter)
add_subdirectory(chardev/demos)

################################################################################
# Distribution of the source code and binaries
################################################################################
# Install sources and binaries
# Copy all files from the source and bin directory to the proper destination
# Leave out git repo related data

install(
    DIRECTORY            
        ${CMAKE_CURRENT_SOURCE_DIR}/adapter            

    DESTINATION .
    COMPONENT source
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE    
)

install(
    DIRECTORY            
        ${CMAKE_CURRENT_SOURCE_DIR}/tools            

    DESTINATION .
    COMPONENT source
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE    
)

install(
    DIRECTORY            
        ${CMAKE_CURRENT_SOURCE_DIR}/eth            

    DESTINATION .
    COMPONENT source
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE    
)

install(
    DIRECTORY            
        ${CMAKE_CURRENT_SOURCE_DIR}/chardev           

    DESTINATION .
    COMPONENT source
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE    
)

install(
    DIRECTORY            
        ${CMAKE_CURRENT_SOURCE_DIR}/qmp           

    DESTINATION .
    COMPONENT source
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE    
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    DESTINATION .
    COMPONENT source
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE
)

install(
    FILES
        CMakeLists.txt
        CMakePresets.json
    DESTINATION .
    COMPONENT source
)

install(
    FILES
        LICENSE
        README.md        
    DESTINATION .
    COMPONENT source
)

install(
    DIRECTORY
        ${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}

    DESTINATION build
    COMPONENT bin
)

###############################################################################
# Packaging
###############################################################################

set(CPACK_GENERATOR "ZIP")

set(CPACK_PACKAGE_DESCRIPTION "binary release of SIL Kit Adapters QEMU")
set(CPACK_PACKAGE_NAME "SilKit-Adapters-QEMU")

set(CPACK_PACKAGE_VENDOR "Vector Informatik")
set(CPACK_PACKAGE_CONTACT "support@vector.com")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}${PACKAGE_FILENAME_SUFFIX}")

include(CPack)

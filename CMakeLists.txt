cmake_minimum_required(VERSION 3.5)
project(VectorSilKitAdapterQemu)

set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)

set(SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(SILKIT_ADAPTER_QEMU_LIBRARY_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

set(SILKIT_ADAPTER_QEMU_VERSION "1.1.1")

if (WIN32)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        # set the default install paths for Windows 32 and 64 bits
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(CMAKE_INSTALL_PREFIX "C:/Program Files/Vector SIL Kit Adapter QEMU ${SILKIT_ADAPTER_QEMU_VERSION}" CACHE PATH "Default install path" FORCE)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Vector SIL Kit Adapter QEMU ${SILKIT_ADAPTER_QEMU_VERSION}" CACHE PATH "Default install path" FORCE)
        endif()
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
endif()

include(common/cmake/RepoTargets.cmake)
include(common/cmake/Building.cmake)

build_common_demo(
    common/demos/SilKitDemoChardevPubSubEchoDevice.cpp
    sil-kit-demo-chardev-echo-device
    ${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}
)

add_subdirectory(adapter)
add_subdirectory(eth/Utility)
add_subdirectory(eth/adapter)
add_subdirectory(eth/demos)

################################################################################
# Distribution of the source code and binaries
################################################################################
# Install sources and binaries
# Copy all files from the source and bin directory to the proper destination
# Leave out git repo related data

install(
    DIRECTORY            
        ${CMAKE_CURRENT_SOURCE_DIR}/adapter
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_CURRENT_SOURCE_DIR}/eth
        ${CMAKE_CURRENT_SOURCE_DIR}/chardev
        ${CMAKE_CURRENT_SOURCE_DIR}/qmp
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
        ${CMAKE_CURRENT_SOURCE_DIR}/tools

    DESTINATION .
    COMPONENT source
    EXCLUDE_FROM_ALL
    REGEX "\.git$" EXCLUDE
    REGEX "\.github$" EXCLUDE
    REGEX "Downloads$" EXCLUDE
)

include(common/cmake/InstallFunctions.cmake)

install_scripts_to(tools
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/setup-host-wsl2-ubuntu.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/build-silkit-qemu-demos-guest.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-silkit-qemu-demos-guest.sh
)

install_scripts_to(eth/demos/CANoe4SW_SE
    ${CMAKE_CURRENT_SOURCE_DIR}/eth/demos/CANoe4SW_SE/run_all.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/eth/demos/CANoe4SW_SE/run.sh
)

install_scripts_to(qmp/demos/CANoe4SW_SE
    ${CMAKE_CURRENT_SOURCE_DIR}/qmp/demos/CANoe4SW_SE/run_all.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/qmp/demos/CANoe4SW_SE/run.sh
)

install_scripts_to(chardev/demos/CANoe4SW_SE
    ${CMAKE_CURRENT_SOURCE_DIR}/chardev/demos/CANoe4SW_SE/run.sh
)

install_root_source_files()

include(GNUInstallDirs)

install_silkit_lib(
    ${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}
    ${SILKIT_ADAPTER_QEMU_LIBRARY_DIRECTORY}
)

install_demo(sil-kit-demo-chardev-echo-device)
install_demo(sil-kit-demo-ethernet-icmp-echo-device)

install_adapter(sil-kit-adapter-qemu)

include(common/cmake/Packaging.cmake)

declare_package(
    "SilKit-Adapter-QEMU"
    "binary release of SIL Kit Adapter QEMU"
)

cmake_minimum_required(VERSION 3.5)
project(VectorSilKitAdapterQemu)

include(common/cmake/RepoTargets.cmake)
include(common/cmake/Building.cmake)

set(SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(SILKIT_ADAPTER_QEMU_LIBRARY_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(SilKitAdapterVersion)
configure_silkit_adapter_version(${PROJECT_NAME})
add_definitions(-DSILKIT_ADAPTER_VERSION="${PROJECT_VERSION}")

if (WIN32)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        # set the default install paths for Windows 32 and 64 bits
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(CMAKE_INSTALL_PREFIX "C:/Program Files/Vector SIL Kit Adapter QEMU ${PROJECT_VERSION}" CACHE PATH "Default install path" FORCE)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Vector SIL Kit Adapter QEMU ${PROJECT_VERSION}" CACHE PATH "Default install path" FORCE)
        endif()
    endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
endif()

build_common_demo(
    common/demos/SilKitDemoChardevPubSubEchoDevice.cpp
    sil-kit-demo-chardev-echo-device
    ${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}
)

enable_address_sanitizer(${ENABLE_ASAN})
enable_undefined_sanitizer(${ENABLE_UBSAN})
enable_thread_sanitizer(${ENABLE_THREADSAN})

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

include(common/cmake/InstallFunctions.cmake)

install_adapter_source(
    ${CMAKE_CURRENT_SOURCE_DIR}/adapter
    ${CMAKE_CURRENT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}/eth
    ${CMAKE_CURRENT_SOURCE_DIR}/chardev
    ${CMAKE_CURRENT_SOURCE_DIR}/qmp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/tools
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

install_scripts_to(tools
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/setup-host-ubuntu.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/build-silkit-qemu-demos-guest.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/run-silkit-qemu-demos-guest.sh
)

install_scripts_to(tools/virt-builder/debian
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/virt-builder/debian/install.100.setup-network.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/virt-builder/debian/install.110.setup-sshd.sh
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
    ${CMAKE_CURRENT_SOURCE_DIR}/chardev/demos/CANoe4SW_SE/run_all.sh
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

if(NOT DEFINED PACKAGE_FILENAME_SUFFIX)
    set(PACKAGE_FILENAME_SUFFIX "-${PROJECT_VERSION}")
endif()

declare_package(
    "SilKit-Adapter-QEMU"
    "binary release of SIL Kit Adapter QEMU"
)

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

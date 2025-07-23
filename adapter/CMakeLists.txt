
add_executable(sil-kit-adapter-qemu
    "SilKitAdapterQemu.cpp"
)

target_link_libraries(sil-kit-adapter-qemu
    PRIVATE
        Utility
        EthSocketToEthControllerAdapter
        asio
        SilKit::SilKit
        sil-kit-adapters-common
)

target_include_directories(sil-kit-adapter-qemu
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(sil-kit-adapter-qemu
	PROPERTIES
    #ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}>
)

copy_sil_kit_to_bin(sil-kit-adapter-qemu ${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY} ${SILKIT_ADAPTER_QEMU_LIBRARY_DIRECTORY})

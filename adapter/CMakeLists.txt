
add_executable(sil-kit-adapter-qemu
    "SilKitAdapterQemu.cpp" 
    "Parsing.cpp"
    "SignalHandler.cpp"
)

target_link_libraries(sil-kit-adapter-qemu
    PRIVATE
        Utility
        ChardevSocketToPubSubAdapter
        EthSocketToEthControllerAdapter
        asio
        SilKit::SilKit
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

add_custom_command(
    TARGET sil-kit-adapter-qemu
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_FILE:SilKit::SilKit> ${SILKIT_ADAPTER_QEMU_LIBRARY_DIRECTORY}/libSilKit.so
)
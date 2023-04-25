
add_executable(SilKitAdapterQemu
    "SilKitAdapterQemu.cpp" 
    "Parsing.cpp" )

target_link_libraries(SilKitAdapterQemu
    PRIVATE
        Utility
        ChardevSocketToPubSubAdapter
        EthSocketToEthControllerAdapter
        asio
        SilKit::SilKit
)

target_include_directories(SilKitAdapterQemu 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(SilKitAdapterQemu 
	PROPERTIES
    #ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}>
)

add_custom_command(
    TARGET SilKitAdapterQemu
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_FILE:SilKit::SilKit> ${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}
)

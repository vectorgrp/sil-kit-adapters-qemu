
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

if(WIN32)
    # Copy the SilKit.dll next to the executables
    add_custom_command(
        TARGET sil-kit-adapter-qemu
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy $<TARGET_FILE:SilKit::SilKit> ${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}/SilKit.dll
    )
    # Copy the SilKit.lib in the lib folder
    get_target_property(SILKIT_DIR_LIB SilKit::SilKit LOCATION) # Get the SilKit.dll full path
    get_filename_component(SILKIT_DIR_LIB ${SILKIT_DIR_LIB} DIRECTORY) # Get the SilKit.dll directory path
    add_custom_command(
        TARGET sil-kit-adapter-qemu
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy ${SILKIT_DIR_LIB}/../lib/SilKit.lib ${SILKIT_ADAPTER_QEMU_LIBRARY_DIRECTORY}/SilKit.lib
    )
else()
    # Copy the libSilKit.so in the lib folder
    add_custom_command(
        TARGET sil-kit-adapter-qemu
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS -E copy $<TARGET_FILE:SilKit::SilKit> ${SILKIT_ADAPTER_QEMU_LIBRARY_DIRECTORY}/libSilKit.so
    )
endif()

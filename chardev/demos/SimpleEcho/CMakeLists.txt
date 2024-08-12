
add_executable(sil-kit-demo-chardev-echo-device
    "SilKitDemoChardevEchoDevice.cpp"
    ${CMAKE_SOURCE_DIR}/adapter/Parsing.cpp
    ${CMAKE_SOURCE_DIR}/adapter/SignalHandler.cpp
)
target_link_libraries(sil-kit-demo-chardev-echo-device
    PRIVATE
        Utility
        SilKit::SilKit
)
set_target_properties(sil-kit-demo-chardev-echo-device 
	PROPERTIES
	#ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}>
)
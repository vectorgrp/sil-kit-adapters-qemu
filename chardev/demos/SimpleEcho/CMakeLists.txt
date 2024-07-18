
add_executable(SilKitDemoChardevEchoDevice
    "SilKitDemoChardevEchoDevice.cpp"
    ${CMAKE_SOURCE_DIR}/adapter/Parsing.cpp
    ${CMAKE_SOURCE_DIR}/adapter/SignalHandler.cpp
)
target_link_libraries(SilKitDemoChardevEchoDevice
    PRIVATE
        Utility
        SilKit::SilKit
)
set_target_properties(SilKitDemoChardevEchoDevice 
	PROPERTIES
	#ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}>
)

add_executable(SilKitDemoEthernetIcmpEchoDevice
    "SilKitDemoEthernetIcmpEchoDevice.cpp"
    Device.hpp
    Device.cpp
    ${CMAKE_SOURCE_DIR}/adapter/Parsing.cpp
    ${CMAKE_SOURCE_DIR}/adapter/SignalHandler.cpp
)
target_link_libraries(SilKitDemoEthernetIcmpEchoDevice
    PRIVATE
        Utility
        SilKit::SilKit
)
set_target_properties(SilKitDemoEthernetIcmpEchoDevice 
	PROPERTIES
	#ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}>
)


add_executable(sil-kit-demo-ethernet-icmp-echo-device
    "SilKitDemoEthernetIcmpEchoDevice.cpp"
    Device.hpp
    Device.cpp
)
target_link_libraries(sil-kit-demo-ethernet-icmp-echo-device
    PRIVATE
        Utility
        SilKit::SilKit
        sil-kit-adapters-common
)
set_target_properties(sil-kit-demo-ethernet-icmp-echo-device 
	PROPERTIES
	#ensure SilKit shared libraries can be loaded
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
    BUILD_RPATH "$ORIGIN"
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTER_QEMU_OUTPUT_DIRECTORY}>
)
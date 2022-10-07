
add_executable(SilKitDemoSpiEchoDevice
    "SilKitDemoSpiEchoDevice.cpp"
    
    
)
target_link_libraries(SilKitDemoSpiEchoDevice
    PRIVATE
        Utility
        SilKit::SilKit
)
set_target_properties(SilKitDemoSpiEchoDevice PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)


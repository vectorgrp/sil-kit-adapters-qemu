
add_executable(SilKitDemoChardevEchoDevice
    "SilKitDemoChardevEchoDevice.cpp"
    
    
)
target_link_libraries(SilKitDemoChardevEchoDevice
    PRIVATE
        Utility
        SilKit::SilKit
)
set_target_properties(SilKitDemoChardevEchoDevice PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)


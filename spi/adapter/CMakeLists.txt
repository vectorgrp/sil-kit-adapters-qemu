
add_executable(SilKitAdapterQemuSpi
    "SilKitAdapterQemuSpi.cpp"
)
target_link_libraries(SilKitAdapterQemuSpi
    PRIVATE
        Utility
        asio
        SilKit::SilKit
)
set_target_properties(SilKitAdapterQemuSpi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET SilKitAdapterQemuSpi
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:SilKit::SilKit,LOCATION> ${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}
)


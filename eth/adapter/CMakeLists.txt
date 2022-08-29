
add_executable(SilKitAdapterQemuEthernet
    "SilKitAdapterQemuEthernet.cpp"
)
target_link_libraries(SilKitAdapterQemuEthernet
    PRIVATE
        Utility
        asio
        SilKit::SilKit
)
set_target_properties(SilKitAdapterQemuEthernet PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET SilKitAdapterQemuEthernet
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:SilKit::SilKit,LOCATION> ${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}
)


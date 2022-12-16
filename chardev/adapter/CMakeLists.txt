
add_executable(SilKitAdapterQemuChardev
    "SilKitAdapterQemuChardev.cpp"
)
target_link_libraries(SilKitAdapterQemuChardev
    PRIVATE
        Utility
        asio
        SilKit::SilKit
)
set_target_properties(SilKitAdapterQemuChardev PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET SilKitAdapterQemuChardev
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:SilKit::SilKit,LOCATION> ${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}
)


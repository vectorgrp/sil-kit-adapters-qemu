
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
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET SilKitAdapterQemuChardev
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:SilKit::SilKit,LOCATION> ${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}
)


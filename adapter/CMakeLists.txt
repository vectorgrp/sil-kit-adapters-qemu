
add_executable(SilKitAdapter
    "SilKitAdapter.cpp" 
    "Parsing.cpp" )
target_link_libraries(SilKitAdapter
    PRIVATE
        Utility
        ChardevSocketToPubSubAdapter
        asio
        SilKit::SilKit
)
target_include_directories(SilKitAdapter 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(SilKitAdapter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET SilKitAdapter
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:SilKit::SilKit,LOCATION> ${SILKIT_ADAPTERS_QEMU_OUTPUT_DIRECTORY}
)



add_executable(ControlSync ControlSync.cpp)
 

target_link_libraries(ControlSync
    PRIVATE
        SilKit::SilKit
)
set_target_properties(ControlSync PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}
)
add_custom_command(
    TARGET ControlSync
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:SilKit::SilKit,LOCATION> "${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}"
)


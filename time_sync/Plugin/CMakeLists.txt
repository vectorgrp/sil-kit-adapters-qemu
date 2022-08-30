
add_library(TimeSyncPlugin SHARED
"TimeSyncPlugin.cpp"
  "qemu-plugin.h" "ExecutionManager.hpp" "Options.hpp")
 
target_link_libraries(TimeSyncPlugin
    PRIVATE
        Utility

)
set_target_properties(TimeSyncPlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET TimeSyncPlugin
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:SilKit::SilKit,LOCATION> "${SK_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}"
)


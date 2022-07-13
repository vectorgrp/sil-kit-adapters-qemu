
add_library(TimeSyncPlugin SHARED
"TimeSyncPlugin.cpp"
  "qemu-plugin.h" "ExecutionManager.hpp" "Options.hpp")
 
target_link_libraries(TimeSyncPlugin
    PRIVATE
        Utility

)
set_target_properties(TimeSyncPlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${IB_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET TimeSyncPlugin
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:IntegrationBus::IntegrationBus,LOCATION> ${IB_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}
)


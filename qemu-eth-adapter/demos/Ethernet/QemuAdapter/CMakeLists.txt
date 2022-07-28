
add_executable(IbDemoEthernetQemuAdapter
    IbDemoEthernetQemuAdapter.cpp
)
target_link_libraries(IbDemoEthernetQemuAdapter
    PRIVATE
        Utility
        asio
        IntegrationBus::IntegrationBus
)
set_target_properties(IbDemoEthernetQemuAdapter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${IB_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)
add_custom_command(
    TARGET IbDemoEthernetQemuAdapter
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy $<TARGET_PROPERTY:IntegrationBus::IntegrationBus,LOCATION> ${IB_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}
)


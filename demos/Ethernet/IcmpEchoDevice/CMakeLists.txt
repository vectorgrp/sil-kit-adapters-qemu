
add_executable(IbDemoEthernetIcmpEchoDevice
    IbDemoEthernetIcmpEchoDevice.cpp
    Device.hpp
    Device.cpp
)
target_link_libraries(IbDemoEthernetIcmpEchoDevice
    PRIVATE
        Utility
        IntegrationBus::IntegrationBus
)
set_target_properties(IbDemoEthernetIcmpEchoDevice PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<$<BOOL:ALWAYS>:${IB_QEMU_DEMOS_RUNTIME_OUTPUT_DIRECTORY}>
)


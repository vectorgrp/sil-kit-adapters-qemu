
add_executable(qemu-ethernet-socket qemu-ethernet-socket.cpp)
target_link_libraries(qemu-ethernet-socket
    PRIVATE
        asio
        fmt
)

add_executable(IbDemoEthernetQemu IbDemoEthernetQemu.cpp QemuSocketClient.hpp)
target_link_libraries(IbDemoEthernetQemu
    PRIVATE
        asio
        fmt
        IntegrationBus::IntegrationBus
)


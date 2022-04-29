
add_executable(DirectQemuSocket
    DirectQemuSocket.cpp

    util/Enums.hpp
    util/Enums.cpp
    util/Exceptions.hpp
    util/Exceptions.cpp
    util/FormattedBuffer.cpp
    util/FormattedBuffer.hpp
    util/ParseResult.hpp
    util/ParseResult.cpp
    util/ReadUintBe.hpp
    util/ReadUintBe.cpp
    util/WriteUintBe.hpp
    util/WriteUintBe.cpp

    util/EthernetAddress.hpp
    util/EthernetAddress.cpp
    util/EthernetHeader.hpp
    util/EthernetHeader.cpp

    util/ArpIp4Packet.hpp
    util/ArpIp4Packet.cpp

    util/Ip4Header.hpp
    util/Ip4Header.cpp
)
target_link_libraries(DirectQemuSocket
    PRIVATE
        asio
        fmt
)
target_include_directories(DirectQemuSocket
    PRIVATE
        util
)

add_executable(IbDemoEthernetQemu
    IbDemoEthernetQemu.cpp
    QemuSocketClient.hpp
)
target_link_libraries(IbDemoEthernetQemu
    PRIVATE
        asio
        fmt
        IntegrationBus::IntegrationBus
)


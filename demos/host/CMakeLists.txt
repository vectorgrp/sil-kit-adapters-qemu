
add_library(
    DemoUtil STATIC

    util/BufferReader.cpp
    util/BufferReader.hpp
    util/BufferWriter.cpp
    util/BufferWriter.hpp
    util/Enums.hpp
    util/Enums.cpp
    util/Exceptions.hpp
    util/Exceptions.cpp
    util/FormattedBuffer.cpp
    util/FormattedBuffer.hpp
    util/InternetChecksum.cpp
    util/InternetChecksum.hpp
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
    util/Icmp4Header.hpp
    util/Icmp4Header.cpp
)
target_include_directories(DemoUtil PUBLIC util)
target_link_libraries(DemoUtil
    PUBLIC
        asio
        fmt
)

add_library(DemoDevice STATIC
    device/Device.hpp
    device/Device.cpp
)
target_include_directories(DemoDevice PUBLIC device)
target_link_libraries(DemoDevice
    PUBLIC
        DemoUtil
)

add_executable(IbDemoEthernetDevice
    IbDemoEthernetDevice.cpp
)
target_link_libraries(IbDemoEthernetDevice
    PRIVATE
        DemoDevice
        IntegrationBus::IntegrationBus
)

add_executable(IbDemoEthernetQemu
    IbDemoEthernetQemu.cpp
)
target_link_libraries(IbDemoEthernetQemu
    PRIVATE
        DemoUtil
        asio
        fmt
        IntegrationBus::IntegrationBus
)


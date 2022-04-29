#include "Device.hpp"

namespace demo {

void Device::Process(asio::const_buffer incomingData)
{
    const auto [ethernetHeader, ethernetPayload] = ParseEthernetHeader(asio::buffer(incomingData));
    fmt::print("{}\n", ethernetHeader);

    switch (ethernetHeader.etherType)
    {
    case EtherType::Arp:
    {
        const auto arpPacket = ParseArpIp4Packet(ethernetPayload);
        fmt::print("{}\n", arpPacket);

        if (arpPacket.operation == ArpOperation::Request)
        {
            if (arpPacket.targetProtocolAddress == _ip4Address)
            {
                EthernetHeader replyEthernetHeader = ethernetHeader;
                replyEthernetHeader.destination = arpPacket.senderHardwareAddress;
                replyEthernetHeader.source = _ethernetAddress;
                fmt::print("Reply: {}\n", replyEthernetHeader);

                ArpIp4Packet replyArpPacket{
                    ArpOperation::Reply,
                    _ethernetAddress,
                    _ip4Address,
                    arpPacket.senderHardwareAddress,
                    arpPacket.senderProtocolAddress,
                };
                fmt::print("Reply: {}\n", replyArpPacket);

                auto reply = AllocateBuffer(incomingData.size());
                auto dst = asio::buffer(reply);

                dst += WriteEthernetHeader(dst, replyEthernetHeader);
                dst += WriteArpIp4Packet(dst, replyArpPacket);

                _sendFrameCallback(std::move(reply));
            }
        }
        break;
    }
    case EtherType::Ip4:
    {
        const auto [ip4Header, ip4Payload] = ParseIp4Header(ethernetPayload);
        fmt::print("{} + {} bytes payload\n", ip4Header, ip4Payload.size());

        switch (ip4Header.protocol)
        {
        case Ip4Protocol::ICMP:
        {
            const auto [icmp4Header, icmp4Payload] = ParseIcmp4Header(ip4Payload);
            fmt::print("{} + {} bytes payload\n", icmp4Header, icmp4Payload.size());

            if (icmp4Header.type == Icmp4Type::EchoRequest)
            {
                if (ip4Header.destinationAddress == _ip4Address)
                {
                    EthernetHeader replyEthernetHeader = ethernetHeader;
                    replyEthernetHeader.destination = replyEthernetHeader.source;
                    replyEthernetHeader.source = _ethernetAddress;
                    fmt::print("Reply: {}\n", replyEthernetHeader);

                    Ip4Header replyIp4Header = ip4Header;
                    replyIp4Header.destinationAddress = replyIp4Header.sourceAddress;
                    replyIp4Header.sourceAddress = _ip4Address;
                    fmt::print("Reply: {}\n", replyIp4Header);

                    Icmp4Header replyIcmp4Header = icmp4Header;
                    replyIcmp4Header.type = Icmp4Type::EchoReply;
                    fmt::print("Reply: {}\n", replyIcmp4Header);

                    auto reply = AllocateBuffer(incomingData.size() + 4);
                    auto dst = asio::buffer(reply);

                    dst += WriteEthernetHeader(dst, replyEthernetHeader);
                    dst += WriteIp4Header(dst, replyIp4Header);
                    auto icmp4Dst = dst;
                    dst += WriteIcmp4Header(dst, replyIcmp4Header);
                    asio::buffer_copy(dst, icmp4Payload);

                    InternetChecksum checksum;
                    checksum.AddBuffer(icmp4Dst);
                    checksum.AddBuffer(icmp4Payload);
                    WriteUintBe(icmp4Dst + 2, checksum.GetChecksum());

                    _sendFrameCallback(std::move(reply));
                }
            }

            break;
        }
        default: break;
        }

        break;
    }
    default: break;
    }
}

} // namespace demo


#include "Enums.hpp"
#include "Exceptions.hpp"
#include "FormattedBuffer.hpp"
#include "ReadUintBe.hpp"
#include "WriteUintBe.hpp"

#include "EthernetAddress.hpp"
#include "EthernetHeader.hpp"

#include "Ip4Address.hpp"

#include "ArpIp4Packet.hpp"
#include "Ip4Header.hpp"

#include <iostream>
#include <memory_resource>
#include <queue>
#include <vector>
#include <optional>
#include <variant>

#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/ts/socket.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace demo {

// QemuSocketClient

class QemuSocketClient
{
    static constexpr EthernetAddress ethernetAddress = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab};
    static constexpr Ip4Address ip4Address = {192, 168, 12, 35};

public:
    QemuSocketClient(asio::io_context& ioContext, std::string_view host, std::string_view service)
        : _socket{ioContext}
        , _resolver{ioContext}
    {
        asio::connect(_socket, _resolver.resolve(host, service));

        DoReceive();
    }

private:
    void DoReceive()
    {
        asio::async_read(
            _socket, asio::buffer(_frameSizeBuffer), [this](const std::error_code ec, const std::size_t bytesReceived) {
                if (ec || bytesReceived != _frameSizeBuffer.size())
                {
                    throw IncompleteReadError{};
                }

                const auto frameSize = ReadUintBe<std::uint32_t>(asio::buffer(_frameSizeBuffer));

                if (frameSize > _frameDataBuffer.size())
                {
                    throw InvalidFrameSizeError{};
                }

                fmt::print("frame with {} bytes announced\n", frameSize);

                asio::async_read(
                    _socket, asio::buffer(_frameDataBuffer, frameSize),
                    [this, frameSize](const std::error_code ec, const std::size_t bytesReceived) {
                        if (ec || bytesReceived != frameSize)
                        {
                            throw IncompleteReadError{};
                        }

                        auto frame_data = AllocateBuffer(frameSize);
                        asio::buffer_copy(asio::buffer(frame_data), asio::buffer(_frameDataBuffer), frameSize);

                        const auto [ethernetHeader, ethernetPayload] = ParseEthernetHeader(asio::buffer(frame_data));
                        fmt::print("{}\n", ethernetHeader);

                        switch (ethernetHeader.etherType)
                        {
                        case EtherType::Arp:
                        {
                            const auto arpPacket = ParseArpIp4Packet(ethernetPayload);
                            fmt::print("{}\n", arpPacket);

                            if (arpPacket.operation == ArpOperation::Request)
                            {
                                if (arpPacket.targetProtocolAddress == ip4Address)
                                {
                                    EthernetHeader replyEthernetHeader = ethernetHeader;
                                    replyEthernetHeader.destination = arpPacket.senderHardwareAddress;
                                    replyEthernetHeader.source = ethernetAddress;
                                    fmt::print("Reply: {}\n", replyEthernetHeader);

                                    ArpIp4Packet replyArpPacket{
                                        .operation = ArpOperation::Reply,
                                        .senderHardwareAddress = ethernetAddress,
                                        .senderProtocolAddress = ip4Address,
                                        .targetHardwareAddress = arpPacket.senderHardwareAddress,
                                        .targetProtocolAddress = arpPacket.senderProtocolAddress,
                                    };
                                    fmt::print("Reply: {}\n", replyArpPacket);

                                    auto reply = AllocateBuffer(frameSize + 4);
                                    std::fill(reply.begin(), reply.end(), std::byte{});

                                    auto dst = asio::buffer(reply) + 4;
                                    const std::size_t replySize = dst.size();

                                    dst += WriteEthernetHeader(dst, replyEthernetHeader);
                                    dst += WriteArpIp4Packet(dst, replyArpPacket);

                                    WriteUintBe<std::uint32_t>(asio::buffer(reply), replySize);

                                    DoSend(asio::buffer(reply));
                                }
                            }
                            break;
                        }
                        case EtherType::Ip4:
                        {
                            const auto [ip4Header, ip4Payload] = ParseIp4Header(ethernetPayload);

                            fmt::print("{} + {} bytes payload", ip4Header, FormattedBuffer{ip4Payload});
                        }
                        default: break;
                        }

                        DoReceive();
                    });
            });
    }

    template <typename ConstBufferSequence>
    void DoSend(const ConstBufferSequence& constBufferSequence)
    {
        _socket.async_send(constBufferSequence, [](std::error_code ec, std::size_t bytesSent) {
            if (ec)
            {
                fmt::print(stderr, "async_send failed: {}\n", ec);
                std::exit(EXIT_FAILURE);
            }

            fmt::print("sent {} bytes\n", bytesSent);
        });
    }

private:
    void ProcessReceiveBuffer() {}

    std::pmr::vector<std::byte> AllocateBuffer(std::size_t size)
    {
        return std::pmr::vector<std::byte>(size, &_memoryResource);
    }

private:
    asio::ip::tcp::socket _socket;
    asio::ip::tcp::resolver _resolver;

    std::array<std::byte, 4> _frameSizeBuffer;
    std::array<std::byte, 4096> _frameDataBuffer;

    std::pmr::unsynchronized_pool_resource _memoryResource;
};

} // namespace demo

int main()
{
#ifdef NDEBUG
    try
    {
#endif
        asio::io_context ioContext;
        demo::QemuSocketClient client{ioContext, "localhost", "12345"};
        ioContext.run();
#ifdef NDEBUG
    }
    catch (const std::exception& exception)
    {
        std::cerr << "error: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
#endif
}

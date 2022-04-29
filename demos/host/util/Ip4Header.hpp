#pragma once

#include "ReadUintBe.hpp"
#include "WriteUintBe.hpp"

#include "EthernetAddress.hpp"
#include "EthernetHeader.hpp"
#include "Ip4Address.hpp"
#include "ParseResult.hpp"

namespace demo {

struct Ip4Header
{
    std::uint16_t totalLength;
    std::uint16_t identification;
    bool dontFragment;
    bool moreFragments;
    std::uint16_t fragmentOffset;
    std::uint8_t timeToLive;
    std::uint8_t protocol;
    std::uint16_t checksum;
    Ip4Address sourceAddress;
    Ip4Address destinationAddress;
};

inline auto ParseIp4Header(asio::const_buffer data) -> ParseResult<Ip4Header>
{
    const auto byte0 = ReadUintBe<std::uint8_t>(data + 0);
    const std::uint8_t ipVersion = (byte0 & 0xF0) >> 4u;
    const std::uint32_t internetHeaderLength = (byte0 & 0x0F) * 4u;

    if (ipVersion != 4)
    {
        throw InvalidIp4PacketError{};
    }

    if (internetHeaderLength < 20 || internetHeaderLength > 60 || data.size() < internetHeaderLength)
    {
        throw InvalidIp4PacketError{};
    }

    // NOTE: We ignore byte1 (typeOfService)

    const auto totalLength = ReadUintBe<std::uint16_t>(data + 2);
    if (internetHeaderLength <= totalLength && data.size() < totalLength)
    {
        throw InvalidIp4PacketError{};
    }

    const auto identification = ReadUintBe<std::uint16_t>(data + 4);

    const auto flagsAndFragmentOffset = ReadUintBe<std::uint16_t>(data + 6);
    if ((flagsAndFragmentOffset & 0b001) != 0)
    {
        throw InvalidIp4PacketError{};
    }

    const bool dontFragment = (flagsAndFragmentOffset & 0b010) != 0;
    const bool moreFragments = (flagsAndFragmentOffset & 0b100) != 0;

    const std::uint16_t fragmentOffset = flagsAndFragmentOffset >> 3u;

    const auto timeToLive = ReadUintBe<std::uint8_t>(data + 8);
    const auto protocol = ReadUintBe<std::uint8_t>(data + 9);
    const auto checksum = ReadUintBe<std::uint16_t>(data + 10);
    const auto sourceAddress = ReadIp4Address(data + 12);
    const auto destinationAddress = ReadIp4Address(data + 16);

    return {
        .header =
            Ip4Header{
                .totalLength = totalLength,
                .identification = identification,
                .dontFragment = dontFragment,
                .moreFragments = moreFragments,
                .fragmentOffset = fragmentOffset,
                .timeToLive = timeToLive,
                .protocol = protocol,
                .checksum = checksum,
                .sourceAddress = sourceAddress,
                .destinationAddress = destinationAddress,
            },
        .remaining = data + internetHeaderLength,
    };
}

} // namespace demo

template <>
struct fmt::formatter<demo::Ip4Header>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::Ip4Header& ip4Header, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(),
                              "Ip4Header(totalLength={},identification={},dontFragment={},"
                              "moreFragments={},fragmentOffset={},timeToLive={},protocol={},checksum={},sourceAddress={"
                              "},destinationAddress={})",
                              ip4Header.totalLength, ip4Header.identification, ip4Header.dontFragment,
                              ip4Header.moreFragments, ip4Header.fragmentOffset, ip4Header.timeToLive,
                              ip4Header.protocol, ip4Header.checksum, ip4Header.sourceAddress,
                              ip4Header.destinationAddress);
    }
};

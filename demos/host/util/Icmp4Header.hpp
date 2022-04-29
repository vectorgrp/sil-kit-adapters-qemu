#pragma once

#include "ReadUintBe.hpp"
#include "WriteUintBe.hpp"
#include "ParseResult.hpp"

#include "Ip4Address.hpp"
#include "Ip4Header.hpp"

namespace demo {

enum struct Icmp4Type : std::uint8_t
{
    EchoReply = 0,
    EchoRequest = 8,
};

struct Icmp4Header
{
    Icmp4Type type;
    std::uint8_t code;
    std::uint16_t checksum;
};

inline auto ParseIcmp4Header(asio::const_buffer data) -> ParseResult<Icmp4Header>
{
    const auto type = ReadUintBe<Icmp4Type>(data + 0);
    const auto code = ReadUintBe<std::uint8_t>(data + 1);
    const auto checksum = ReadUintBe<std::uint16_t>(data + 2);

    return {
        Icmp4Header{
            type,
            code,
            checksum,
        },
        data + 4,
    };
}

inline auto WriteIcmp4Header(const asio::mutable_buffer target, const Icmp4Header& icmp4Header) -> std::size_t
{
    BufferWriter writer{target};

    writer.WriteBe(icmp4Header.type);
    writer.WriteBe(icmp4Header.code);
    writer.WriteBe(std::uint16_t(0));

    return writer.GetOffset();
}

} // namespace demo

template <>
struct fmt::formatter<demo::Icmp4Type>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::Icmp4Type& icmp4Type, FormatContext& ctx) -> decltype(ctx.out())
    {
        switch (icmp4Type)
        {
        case demo::Icmp4Type::EchoReply: return fmt::format_to(ctx.out(), "Icmp4Type::EchoReply");
        case demo::Icmp4Type::EchoRequest: return fmt::format_to(ctx.out(), "Icmp4Type::EchoRequest");
        }
        return fmt::format_to(ctx.out(), "Icmp4Type({})", ToUnderlying(icmp4Type));
    }
};

template <>
struct fmt::formatter<demo::Icmp4Header>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::Icmp4Header& header, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "Icmp4Header(type={},code={},checksum={})", header.type, header.code,
                              header.checksum);
    }
};

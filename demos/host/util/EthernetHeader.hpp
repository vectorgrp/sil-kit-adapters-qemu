#pragma once

#include "Enums.hpp"
#include "ReadUintBe.hpp"
#include "WriteUintBe.hpp"

#include "ParseResult.hpp"
#include "EthernetAddress.hpp"

#include <optional>

#include <cstdint>

#include <asio/ts/buffer.hpp>

#include <fmt/format.h>

namespace demo {

enum struct EtherType : std::uint16_t
{
    Ip4 = 0x0800,
    Arp = 0x0806,

    Vlan_802_1q = 0x8100,
    Vlan_802_1ad = 0x88A8,
};

struct EthernetVlanTag
{
    EtherType tpid;
    std::uint16_t data;
};

inline auto WriteEthernetVlanTag(asio::mutable_buffer target, const EthernetVlanTag& ethernetVlanTag) -> std::size_t
{
    target += WriteUintBe(target, ethernetVlanTag.tpid);
    target += WriteUintBe(target + 2, ethernetVlanTag.data);
    return 4;
}

static_assert(sizeof(EthernetVlanTag) == 4 && std::is_trivial<EthernetVlanTag>::value);

struct EthernetHeader
{
    EthernetAddress destination;
    EthernetAddress source;
    std::optional<EthernetVlanTag> vlanTag8021ad;
    std::optional<EthernetVlanTag> vlanTag8021q;
    EtherType etherType;
};

inline auto ParseEthernetHeader(asio::const_buffer data) -> ParseResult<EthernetHeader>
{
    asio::const_buffer remaining = asio::buffer(data);

    EthernetHeader ethernetHeader = {};

    ethernetHeader.destination = ReadEthernetAddress(remaining);
    remaining += 6;

    ethernetHeader.source = ReadEthernetAddress(remaining);
    remaining += 6;

    if (const auto tpid = ReadUintBe<EtherType>(remaining); tpid == EtherType::Vlan_802_1ad)
    {
        ethernetHeader.vlanTag8021ad = EthernetVlanTag{.tpid = tpid, .data = ReadUintBe<std::uint16_t>(remaining + 2)};
        remaining += 4;
    }

    if (const auto tpid = ReadUintBe<EtherType>(remaining); tpid == EtherType::Vlan_802_1q)
    {
        ethernetHeader.vlanTag8021q = EthernetVlanTag{.tpid = tpid, .data = ReadUintBe<std::uint16_t>(remaining + 2)};
        remaining += 4;
    }

    ethernetHeader.etherType = ReadUintBe<EtherType>(remaining);
    remaining += 2;

    return {.header = ethernetHeader, .remaining = remaining};
}

inline auto WriteEthernetHeader(const asio::mutable_buffer target, EthernetHeader ethernetHeader) -> std::size_t
{
    asio::mutable_buffer dst = target;

    dst += WriteEthernetAddress(dst, ethernetHeader.destination);
    dst += WriteEthernetAddress(dst, ethernetHeader.source);

    if (ethernetHeader.vlanTag8021ad)
    {
        dst += WriteEthernetVlanTag(dst, ethernetHeader.vlanTag8021ad.value());
    }

    if (ethernetHeader.vlanTag8021q)
    {
        dst += WriteEthernetVlanTag(dst, ethernetHeader.vlanTag8021q.value());
    }

    dst += WriteUintBe(dst, ethernetHeader.etherType);

    return target.size() - dst.size();
}

} // namespace demo

template <>
struct fmt::formatter<demo::EtherType>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::EtherType& etherType, FormatContext& ctx) -> decltype(ctx.out())
    {
        switch (etherType)
        {
        case demo::EtherType::Ip4: return fmt::format_to(ctx.out(), "EtherType::Ip4");
        case demo::EtherType::Arp: return fmt::format_to(ctx.out(), "EtherType::Arp");
        case demo::EtherType::Vlan_802_1q: return fmt::format_to(ctx.out(), "EtherType::Vlan_802_1q");
        case demo::EtherType::Vlan_802_1ad: return fmt::format_to(ctx.out(), "EtherType::Vlan_802_1ad");
        }
        return fmt::format_to(ctx.out(), "EtherType({})", ToUnderlying(etherType));
    }
};

template <>
struct fmt::formatter<demo::EthernetVlanTag>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::EthernetVlanTag& ethernetVlanTag, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "EthernetVlanTag(tpid={},data={})", ethernetVlanTag.tpid,
                              ethernetVlanTag.data);
    }
};

template <>
struct fmt::formatter<demo::EthernetHeader>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::EthernetHeader& ethernetFrame, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "EthernetHeader(destination={},source={},etherType={})",
                              ethernetFrame.destination, ethernetFrame.source, ethernetFrame.etherType);
    }
};

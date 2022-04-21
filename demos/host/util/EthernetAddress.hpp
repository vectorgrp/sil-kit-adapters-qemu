#pragma once

#include "Exceptions.hpp"
#include "FormattedBuffer.hpp"

#include <array>

#include <asio/ts/buffer.hpp>

#include <fmt/format.h>

namespace demo {

struct EthernetAddress
{
    std::array<std::uint8_t, 6> data;
};

inline bool operator==(const EthernetAddress& lhs, const EthernetAddress& rhs)
{
    return lhs.data == rhs.data;
}

inline auto ReadEthernetAddress(asio::const_buffer buffer) -> EthernetAddress
{
    EthernetAddress address = {};
    if (asio::buffer_copy(asio::buffer(address.data), buffer) != 6)
    {
        throw InvalidBufferSize{};
    }
    return address;
}

inline auto WriteEthernetAddress(asio::mutable_buffer target, const EthernetAddress& ethernetAddress) -> std::size_t
{
    if (asio::buffer_copy(target, asio::buffer(ethernetAddress.data)) != 6)
    {
        throw InvalidBufferSize{};
    }

    return 6;
}

} // namespace demo

template <>
struct fmt::formatter<demo::EthernetAddress>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::EthernetAddress& ethernetAddress, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "EthernetAddress({})",
                              demo::FormattedBuffer{asio::buffer(ethernetAddress.data)});
    }
};

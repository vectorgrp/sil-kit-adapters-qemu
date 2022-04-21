#pragma once

#include "Exceptions.hpp"

#include <array>

#include <asio/ts/net.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace demo {

struct Ip4Address
{
    std::array<std::uint8_t, 4> data;
};

inline bool operator==(const Ip4Address& lhs, const Ip4Address& rhs)
{
    return lhs.data == rhs.data;
}

inline auto ReadIp4Address(asio::const_buffer buffer) -> Ip4Address
{
    Ip4Address address = {};
    if (asio::buffer_copy(asio::buffer(address.data), buffer) != 4)
    {
        throw InvalidBufferSize{};
    }
    return address;
}

inline auto WriteIp4Address(asio::mutable_buffer target, const Ip4Address& ip4Address) -> std::size_t
{
    if (asio::buffer_copy(target, asio::buffer(ip4Address.data)) != 4)
    {
        throw InvalidBufferSize{};
    }
    return 4;
}

} // namespace demo

template <>
struct fmt::formatter<demo::Ip4Address>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::Ip4Address& ip4Address, FormatContext& ctx) -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", asio::ip::address_v4{ip4Address.data});
    }
};

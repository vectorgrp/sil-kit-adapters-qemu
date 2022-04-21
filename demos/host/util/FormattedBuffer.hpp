#pragma once

#include <asio/ts/buffer.hpp>

#include <fmt/format.h>

namespace demo {

struct FormattedBuffer
{
    asio::const_buffer buffer;
};

} // namespace demo

template <>
struct fmt::formatter<demo::FormattedBuffer>
{
    constexpr auto parse(fmt::format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const demo::FormattedBuffer& formattedBuffer, FormatContext& ctx) -> decltype(ctx.out())
    {
        auto it = ctx.out();
        const auto bytes = static_cast<const std::byte*>(formattedBuffer.buffer.data());

        for (std::size_t index = 0; index < formattedBuffer.buffer.size(); ++index)
        {
            if (index > 0)
            {
                it = fmt::format_to(it, ":");
            }
            it = fmt::format_to(it, "{:02x}", bytes[index]);
        }
        return it;
    }
};

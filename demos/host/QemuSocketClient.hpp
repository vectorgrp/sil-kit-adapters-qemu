
#include <iostream>
#include <array>
#include <string_view>
#include <queue>
#include <iterator>
#include <stdexcept>
#include <memory_resource>
#include <optional>

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cinttypes>

#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/ts/socket.hpp>

struct IncompleteReadError : std::runtime_error
{
    IncompleteReadError()
        : std::runtime_error("incomplete read")
    {
    }
};

struct InvalidFrameSizeError : std::runtime_error
{
    InvalidFrameSizeError()
        : std::runtime_error("invalid frame size")
    {
    }
};

class QemuSocketClient
{
public:
    QemuSocketClient(asio::io_context& io_context, std::string_view host, std::string_view service,
                     std::function<void(std::vector<uint8_t>)> onNewFrameHandler)
        : _socket{io_context}
        , _resolver{io_context}
        , mOnNewFrameHandler(onNewFrameHandler)
    {
        asio::connect(_socket, _resolver.resolve(host, service));
        std::cout << "connect success" << std::endl;
        do_receive();
    }

    void do_send(const std::vector<uint8_t>& data)
    {
        _socket.async_send(asio::buffer(data.data(), data.size()), [](std::error_code ec, std::size_t bytes_sent) {
            if (ec)
            {
                std::cerr << "async_send failed: " << ec << std::endl;
                std::exit(EXIT_FAILURE);
            }

            std::cout << "sent vib frame" << bytes_sent << " bytes" << std::endl;
        });
    }

private:
    void do_receive()
    {
        asio::async_read(_socket, asio::buffer(_frame_size_buffer.data(), _frame_size_buffer.size()),
                         [this](const std::error_code ec, const std::size_t bytes_received) {
                             if (ec || bytes_received != _frame_size_buffer.size())
                             {
                                 throw IncompleteReadError{};
                             }

                             std::uint32_t frame_size = 0;
                             for (unsigned byte_index = 0; byte_index < 4; ++byte_index)
                             {
                                 frame_size |= static_cast<std::uint32_t>(_frame_size_buffer[4 - 1 - byte_index])
                                               << (byte_index * 8u);
                             }

                             if (frame_size > _frame_data_buffer.size())
                             {
                                 throw InvalidFrameSizeError{};
                             }

                             asio::async_read(
                                 _socket, asio::buffer(_frame_data_buffer.data(), frame_size),
                                 [this, frame_size](const std::error_code ec, const std::size_t bytes_received) {
                                     if (ec || bytes_received != frame_size)
                                     {
                                         throw IncompleteReadError{};
                                     }

                                     auto frame_data = make_frame_data(frame_size);
                                     asio::buffer_copy(
                                         asio::buffer(frame_data),
                                         asio::buffer(_frame_data_buffer.data(), _frame_data_buffer.size()),
                                         frame_size);

                                     print_frame_data(frame_data);

                                     mOnNewFrameHandler(frame_data);
                                     do_receive();
                                 });
                         });
    }

    void print_frame_data(std::vector<uint8_t>& frame_data)
    {
        std::cout << "frame data ";
        constexpr auto nibble_to_hex_char = [](const unsigned nibble) {
            if (nibble < 10)
                return '0' + nibble;
            else
                return 'a' + (nibble - 10u);
        };

        auto it = std::ostream_iterator<char>{std::cout};

        std::size_t index = 0;
        for (auto byte : frame_data)
        {
            if (index++ > 0)
                *it++ = ':';
            *it++ = nibble_to_hex_char((static_cast<unsigned>(byte) & 0xF0) >> 4u);
            *it++ = nibble_to_hex_char((static_cast<unsigned>(byte) & 0x0F) >> 0u);
        }
    }

    std::vector<uint8_t> make_frame_data(std::size_t size)
    {
        std::vector<uint8_t> frame_data;
        frame_data.resize(size);
        return frame_data;
    }

    asio::ip::tcp::socket _socket;
    asio::ip::tcp::resolver _resolver;

    std::array<uint8_t, 4> _frame_size_buffer;
    std::array<uint8_t, 4096> _frame_data_buffer;

    std::function<void(std::vector<uint8_t>)> mOnNewFrameHandler;
};

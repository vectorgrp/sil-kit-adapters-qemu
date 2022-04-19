
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

struct InvalidEthernetFrameError : std::runtime_error
{
    InvalidEthernetFrameError()
        : std::runtime_error("invalid ethernet frame")
    {
    }
};

/*
struct EthernetAddress
{
    std::array<std::byte, 6> data;
};

std::ostream& operator<<(std::ostream& o, const EthernetAddress& ethernetAddress)
{
    return o << "EthernetAddress{...TODO...}";
}

class EthernetFrameReader
{
public:
    explicit EthernetFrameReader(asio::const_buffer data)
        : _data{data}
    {
    }

    auto GetDestination() const -> EthernetAddress;
    auto GetSource() const -> EthernetAddress;

private:
    asio::const_buffer _data;
};

auto EthernetFrameReader::GetDestination() const -> EthernetAddress
{
    EthernetAddress address;
    if (asio::buffer_copy(asio::buffer(address.data), _data) != 6)
    {
        throw InvalidEthernetFrameError{};
    }
    return address;
}

auto EthernetFrameReader::GetSource() const -> EthernetAddress
{
    EthernetAddress address;
    if (asio::buffer_copy(asio::buffer(address.data), _data + 6) != 6)
    {
        throw InvalidEthernetFrameError{};
    }
    return address;
}

class EthernetFrame
{
public:
    auto GetDestination() const -> EthernetAddress;
    void SetDestination(EthernetAddress address);

    auto GetSource() const -> EthernetAddress;
    void SetSource(EthernetAddress address);

private:
    std::pmr::vector<std::byte> _data;
};

auto EthernetFrame::GetDestination() const -> EthernetAddress
{
    EthernetAddress address;
    if (asio::buffer_copy(asio::buffer(address), asio::buffer(_data)) != 6)
    {
        throw InvalidEthernetFrameError{};
    }
    return address;
}

void EthernetFrame::SetDestination(const EthernetAddress address)
{
}

auto EthernetFrame::GetSource() const -> EthernetAddress
{
    EthernetAddress address;
    if (asio::buffer_copy(asio::buffer(address), asio::buffer(_data) + 6) != 6)
    {
        throw InvalidEthernetFrameError{};
    }
    return address;
}

void EthernetFrame::SetSource(const EthernetAddress address)
{
}
*/

class QemuSocketClient
{
public:
    QemuSocketClient(asio::io_context& io_context, std::string_view host, std::string_view service)
        : _socket{io_context}
        , _resolver{io_context}
    {
        asio::connect(_socket, _resolver.resolve(host, service));

        do_receive();
    }

private:
    void do_receive()
    {
        asio::async_read(_socket, asio::buffer(_frame_size_buffer),
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

                             std::cout << "frame with " << frame_size << " bytes announced" << std::endl;

                             asio::async_read(
                                 _socket, asio::buffer(_frame_data_buffer, frame_size),
                                 [this, frame_size](const std::error_code ec, const std::size_t bytes_received) {
                                     if (ec || bytes_received != frame_size)
                                     {
                                         throw IncompleteReadError{};
                                     }

                                     auto frame_data = make_frame_data(frame_size);
                                     asio::buffer_copy(asio::buffer(frame_data), asio::buffer(_frame_data_buffer),
                                                       frame_size);

                                     std::cout << "frame data ";
                                     {
                                         constexpr auto nibble_to_hex_char = [](const unsigned nibble) {
                                             if (nibble < 10)
                                                 return '0' + nibble;
                                             else
                                                 return 'a' + (nibble - 10u);
                                         };

                                         auto it = std::ostream_iterator<char>{std::cout};
                                         //auto it = std::ostreambuf_iterator{std::cout};
                                         std::size_t index = 0;
                                         for (const std::byte byte : frame_data)
                                         {
                                             if (index++ > 0)
                                                 *it++ = ':';
                                             *it++ = nibble_to_hex_char((static_cast<unsigned>(byte) & 0xF0) >> 4u);
                                             *it++ = nibble_to_hex_char((static_cast<unsigned>(byte) & 0x0F) >> 0u);
                                         }
                                     }
                                     std::cout << std::endl;

                                     _incoming_frames.emplace(std::move(frame_data));

                                     do_receive();
                                 });
                         });
    }

    template <typename ConstBufferSequence>
    void do_send(const ConstBufferSequence& const_buffer_sequence)
    {
        _socket.async_send(const_buffer_sequence, [](std::error_code ec, std::size_t bytes_sent) {
            if (ec)
            {
                std::cerr << "async_send failed: " << ec << std::endl;
                std::exit(EXIT_FAILURE);
            }

            std::cout << "sent " << bytes_sent << " bytes" << std::endl;
        });
    }

private:
    void process_receive_buffer() {}

    std::pmr::vector<std::byte> make_frame_data(std::size_t size)
    {
        return std::pmr::vector<std::byte>(&_memory_resource);
    }

private:
    asio::ip::tcp::socket _socket;
    asio::ip::tcp::resolver _resolver;

    std::array<std::byte, 4> _frame_size_buffer;
    std::array<std::byte, 4096> _frame_data_buffer;

    std::pmr::unsynchronized_pool_resource _memory_resource;
    std::queue<std::pmr::vector<std::byte>> _incoming_frames;
};

int main()
{
    try
    {
        asio::io_context ioContext;
        QemuSocketClient client{ioContext, "localhost", "12345"};
        ioContext.run();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "error: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
}

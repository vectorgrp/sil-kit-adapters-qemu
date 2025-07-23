// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include <vector>
#include <functional>

#include "Ip4Address.hpp"
#include "EthernetAddress.hpp"

namespace demo {

class Device
{
public:
public:
    Device(EthernetAddress ethernetAddress, Ip4Address ip4Address,
           std::function<void(std::vector<std::uint8_t>)> sendFrameCallback)
        : _ethernetAddress{ethernetAddress}
        , _ip4Address{ip4Address}
        , _sendFrameCallback{std::move(sendFrameCallback)}
    {
    }

public:
    void Process(asio::const_buffer incomingData);

public:
    std::vector<std::uint8_t> AllocateBuffer(std::size_t size) { return std::vector<std::uint8_t>(size); }

private:
    EthernetAddress _ethernetAddress;
    Ip4Address _ip4Address;
    std::function<void(std::vector<std::uint8_t>)> _sendFrameCallback;
};

} // namespace demo

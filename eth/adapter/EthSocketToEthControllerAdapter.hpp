// SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
// SPDX-License-Identifier: MIT

#pragma once

#include <set>

#include "silkit/SilKit.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/write.hpp"
#include "WriteUintBe.hpp"

#include "asio/generic/stream_protocol.hpp"

namespace adapters {
namespace ethernet {

class EthSocketToEthControllerAdapter
{
    using EthernetFrame = SilKit::Services::Ethernet::EthernetFrame;
    using IEthernetController = SilKit::Services::Ethernet::IEthernetController;
    using EthernetFrameTransmitEvent = SilKit::Services::Ethernet::EthernetFrameTransmitEvent;
    using EthernetFrameEvent = SilKit::Services::Ethernet::EthernetFrameEvent;
    using EthernetTransmitStatus = SilKit::Services::Ethernet::EthernetTransmitStatus;

public:
    EthSocketToEthControllerAdapter(SilKit::IParticipant* participant, asio::io_context& io_context,
                                    const std::string& host, const std::string& service,
                                    const std::string& ethernetControllerName, const std::string& ethernetNetworkName,
                                    bool enableDomainSockets);

    template <class container>
    void SendEthernetFrameToQemu(const container& data)
    {
        std::array<std::uint8_t, 4> frameSizeBytes = {};
        demo::WriteUintBe(asio::buffer(frameSizeBytes), std::uint32_t(data.size()));

        asio::write(_socket, asio::buffer(frameSizeBytes));
        asio::write(_socket, asio::buffer(data.data(), data.size()));
    }

private:
    void DoReceiveFrameFromQemu();

    asio::generic::stream_protocol::socket _socket;
    SilKit::Services::Logging::ILogger* _logger;

    std::array<uint8_t, 4> _frame_size_buffer = {};
    std::array<uint8_t, 4096> _frame_data_buffer = {};

    SilKit::Services::Ethernet::IEthernetController* _ethController;
    std::function<void(std::vector<uint8_t>)> _onNewFrameHandler;
};

std::unique_ptr<EthSocketToEthControllerAdapter> parseEthernetSocketArgument(
    char* arg, std::set<std::string>& alreadyProvidedSockets, const std::string& participantName,
    asio::io_context& ioContext, SilKit::IParticipant* participant, SilKit::Services::Logging::ILogger* logger,
    const bool isTcpSocket);

} // namespace ethernet
} // namespace adapters

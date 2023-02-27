// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/ethernet/all.hpp"
#include "silkit/services/logging/all.hpp"
#include "silkit/services/ethernet/string_utils.hpp"
#include "WriteUintBe.hpp"

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
                                      const std::string ethernetControllerName, const std::string ethernetNetworkName);

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

    asio::ip::tcp::socket _socket;
    SilKit::Services::Logging::ILogger* _logger;

    std::array<uint8_t, 4> _frame_size_buffer = {};
    std::array<uint8_t, 4096> _frame_data_buffer = {};

    SilKit::Services::Ethernet::IEthernetController* _ethController;
    std::function<void(std::vector<uint8_t>)> _onNewFrameHandler;
};

EthSocketToEthControllerAdapter* parseEthernetSocketArgument(char* arg, std::set<std::string>& alreadyProvidedSockets,
                                                               const std::string& participantName,
                                                               asio::io_context& ioContext,
                                                               SilKit::IParticipant* participant,
                                                               SilKit::Services::Logging::ILogger* logger);

} // namespace ethernet
} // namespace adapters

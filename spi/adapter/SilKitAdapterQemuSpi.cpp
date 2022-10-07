// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "Exceptions.hpp"
#include "WriteUintBe.hpp"

#include <asio/ts/buffer.hpp>
#include <asio/ts/io_context.hpp>
#include <asio/ts/net.hpp>

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/PubSub/all.hpp"
#include "silkit/util/serdes/Serialization.hpp"

using namespace SilKit::Services::PubSub;

using namespace std::chrono_literals;

class QemuSocketTransmitter
{
public:
    QemuSocketTransmitter(asio::io_context& io_context, const std::string& host, const std::string& service,
                          IDataPublisher* publisher,
                          IDataSubscriber* subscriber)
        : _socket{io_context}
        , _publisher{publisher}
        , _subscriber{subscriber}
    {
        asio::connect(_socket, asio::ip::tcp::resolver{io_context}.resolve(host, service));
        std::cout << "connect success" << std::endl;
        _subscriber->SetDataMessageHandler(
            [&](SilKit::Services::PubSub::IDataSubscriber* subscriber, const DataMessageEvent& dataMessageEvent) {
                //remove size which is added by CANoe.
                const auto data =
                    std::string_view((const char*)dataMessageEvent.data.data() + 4, dataMessageEvent.data.size() - 4);
                std::cout << "SIL Kit >> QEMU: " << data << std::endl;
                SendToQemu(data);
            });
        DoReceiveFrameFromQemu();
    }

    template<class vector_like_container>
    void SendToQemu(const vector_like_container& data)
    {
        asio::write(_socket, asio::buffer(data.data(), data.size()));
    }

private:
    void DoReceiveFrameFromQemu()
    {
        asio::async_read_until(_socket, asio::dynamic_buffer(_data_buffer), '\n',
                               [this](const std::error_code ec, const std::size_t bytes_received) {
                                   if( ec )
                                       throw demo::IncompleteReadError{};

                                   auto eol_it = std::find(_data_buffer.begin(), _data_buffer.end(), '\n');
                                   if( eol_it==_data_buffer.end() )
                                       throw demo::IncompleteReadError{};

                                   auto line_len = eol_it - _data_buffer.begin();

                                   std::cout << "QEMU >> SIL Kit: "
                                             << std::string_view(_data_buffer.data(), line_len)
                                             << std::endl;
                                   
                                   //Prefix data with big endian size. This is for CANoe.
                                   std::vector<uint8_t> dataToPublish;
                                   dataToPublish.reserve(4 + line_len);
                                   for (int byte_index = 0; byte_index < 4; byte_index++)
                                       dataToPublish.push_back( (line_len >> (8 * byte_index)) & 0xFF);
                                   std::copy(_data_buffer.begin(), eol_it,
                                             std::back_inserter(dataToPublish));
                                   _publisher->Publish(SilKit::Util::Span<uint8_t>(dataToPublish));

                                   //clean buffer of the read line; asio doesn't do it on next invocation
                                   _data_buffer.erase(_data_buffer.begin(), eol_it + 1);

                                   DoReceiveFrameFromQemu();
                               });
    }

private:
    asio::ip::tcp::socket _socket;

    std::vector<char> _data_buffer = {};
    IDataPublisher* _publisher = nullptr;
    IDataSubscriber* _subscriber = nullptr;

    std::function<void(std::vector<uint8_t>)> _onNewFrameHandler;
};

int main(int argc, char** argv)
{
    const std::string participantConfigurationString =
        R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": "Info" } ] } })";

    const std::string qemuHostname = [argc, argv]() -> std::string {
        if (argc >= 2)
        {
            return argv[1];
        }
        return "localhost";
    }();
    const std::string qemuService = [argc, argv]() -> std::string {
        if (argc >= 3)
        {
            return argv[2];
        }
        return "23456";
    }();

    asio::io_context ioContext;

    const std::string participantName = "SPIAdapter";
    const std::string registryURI = "silkit://localhost:8501";

    const std::string publisherName = participantName + "_pub";
    const std::string subscriberName = participantName + "_sub";

    const auto create_pubsubspec = [](const std::string& topic_name, const std::string& instance,
                                SilKit::Services::MatchingLabel::Kind matching_mode) {
        PubSubSpec r(topic_name, SilKit::Util::SerDes::MediaTypeData());
        r.AddLabel("VirtualNetwork", "Default", matching_mode);
        r.AddLabel("Namespace", "Namespace", matching_mode);
        // Uncomment next line if you have a meaningful instance to filter, but this makes it necessary to "know"
        // the instance of the sender, see invocations of "create_pubsubspec", as well as make CANoe a disturbance
        // in this (this is the DO's object name)
        //r.AddLabel("Instance", instance , matching_mode);
        return r;
    };

    const PubSubSpec subDataSpec =
        create_pubsubspec("qemuInbound", "SPIDevice", SilKit::Services::MatchingLabel::Kind::Mandatory);


    const PubSubSpec pubDataSpec =
        create_pubsubspec("qemuOutbound", participantName, SilKit::Services::MatchingLabel::Kind::Optional);


    try
    {
        auto participantConfiguration =
            SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);

        //TODO: create Data Publisher/Subscriber
        auto dataPublisher = participant->CreateDataPublisher(publisherName, pubDataSpec);

        auto dataSubscriber = participant->CreateDataSubscriber(
            subscriberName, subDataSpec,
            [&](SilKit::Services::PubSub::IDataSubscriber* subscriber, const DataMessageEvent& dataMessageEvent) {
                std::cout << "SIL Kit >> QEMU (default): "
                          << std::string_view(reinterpret_cast<const char*>(dataMessageEvent.data.data()),
                                              dataMessageEvent.data.size())
                          << std::endl;
                ;
            });

        QemuSocketTransmitter transmitter{ioContext, qemuHostname, qemuService, dataPublisher, dataSubscriber};

        ioContext.run();

        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
    }
    catch (const SilKit::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return -2;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return -3;
    }

    return 0;
}

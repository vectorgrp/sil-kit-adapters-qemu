// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

#include "Exceptions.hpp"
#include "WriteUintBe.hpp"

#include <asio/ts/buffer.hpp>
#include <asio/ts/io_context.hpp>
#include <asio/ts/net.hpp>

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/pubsub/all.hpp"
#include "silkit/util/serdes/Serialization.hpp"

using namespace SilKit::Services::PubSub;

using namespace std::chrono_literals;

class QemuSocketTransmitter
{
  typedef std::string string;
public:

  template<class vector_like_container>
  void SendToQemu( const vector_like_container& data )
  {
    asio::write( _socket , asio::buffer( data.data() , data.size() ) );
  }

  QemuSocketTransmitter(asio::io_context& io_context, const string& host, const string& service,
                         const string& publisherName, const string& subscriberName,  
                         const PubSubSpec& pubDataSpec, const PubSubSpec& subDataSpec, 
                         SilKit::IParticipant* participant)
    : _socket { io_context }
    , _publisher { participant->CreateDataPublisher( publisherName, pubDataSpec ) }
    , _subscriber{ participant->CreateDataSubscriber( subscriberName, subDataSpec,
              [&](SilKit::Services::PubSub::IDataSubscriber* subscriber, const DataMessageEvent& dataMessageEvent) {
                  //remove size which is added by CANoe.
                  const auto data =
                      std::string_view((const char*)dataMessageEvent.data.data() + 4, dataMessageEvent.data.size() - 4);
                  std::cout << "SIL Kit >> QEMU: " << data << std::endl;
                  SendToQemu(data);
              })}
  {
    asio::connect( _socket , asio::ip::tcp::resolver { io_context }.resolve( host , service ) );
    std::cout << "connect success" << std::endl;
    DoReceiveFrameFromQemu();
  }


private:
  void DoReceiveFrameFromQemu()
  {
      asio::async_read_until(_socket, asio::dynamic_buffer(_data_buffer_in), '\n',
                            [ this ]( const std::error_code ec , const std::size_t bytes_received ) {
                              if( ec )
                                throw demo::IncompleteReadError {};

                              auto eol_it = std::find(_data_buffer_in.begin(), _data_buffer_in.end(), '\n');
                              if (eol_it == _data_buffer_in.end())
                                throw demo::IncompleteReadError {};

                              auto line_len = eol_it - _data_buffer_in.begin() + 1;

                              std::cout << "QEMU >> SIL Kit: " << std::string_view(_data_buffer_in.data(), line_len);

                              //Prefix data with big endian size. This is for CANoe.
                              _data_buffer_out.clear();
                              _data_buffer_out.reserve(4 + line_len);
                              for( int byte_index = 0; byte_index < 4; byte_index++ )
                                  _data_buffer_out.push_back((line_len >> (8 * byte_index)) & 0xFF);
                              std::copy(_data_buffer_in.begin(), eol_it + 1, std::back_inserter(_data_buffer_out));
                              _publisher->Publish(SilKit::Util::Span<uint8_t>(_data_buffer_out));

                              //clean buffer of the read line; asio doesn't do it on next invocation
                              _data_buffer_in.erase( _data_buffer_in.begin() , eol_it + 1 );

                              DoReceiveFrameFromQemu();
                            } );
  }

private:
  asio::ip::tcp::socket _socket;

  std::vector<char> _data_buffer_in = {};
  std::vector<uint8_t> _data_buffer_out = {};
  IDataPublisher* _publisher;
  IDataSubscriber* _subscriber;
};

int main(int argc, char** argv)
{
    const std::string participantConfigurationString =
        R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": "Info" } ] } })";

    const auto getArgDefault = [ argc , argv ](const std::string& argument, const std::string& defaultValue)-> auto{
        return [argc, argv, argument, defaultValue]() -> std::string {
            auto found = std::find_if(argv, argv + argc, [argument](const char* arg) -> bool {
                return arg == argument;
            });
            if (found != argv + argc && found + 1 != argv + argc)
                return *(found + 1);
            return defaultValue;
        };
    };

    const std::string qemuHostname = getArgDefault("--hostname","localhost")();
    const std::string qemuService = getArgDefault("--port","23456")();

    const auto qemuInboundTopicName = getArgDefault("--qemuinbound","qemuInbound")();
    const auto qemuOutboundTopicName = getArgDefault("--qemuoutbound","qemuOutbound")();

    asio::io_context ioContext;

    const std::string participantName = getArgDefault("--name","ChardevAdapter")();

    const std::string registryURI = "silkit://localhost:8501";

    const std::string publisherName = participantName + "_pub";
    const std::string subscriberName = participantName + "_sub";

    const auto create_pubsubspec = [](const std::string& topic_name,
                                SilKit::Services::MatchingLabel::Kind matching_mode) {
        PubSubSpec r(topic_name, SilKit::Util::SerDes::MediaTypeData());
        r.AddLabel("VirtualNetwork", "Default", matching_mode);
        r.AddLabel("Namespace", "Namespace", matching_mode);
        // Next lines would filter either CANoe's or the other participant's, so we don't add it.
        //instance = "Observed";
        //instance = "Stimulate";
        //r.AddLabel("Instance", instance, matching_mode);
        return r;
    };

    const PubSubSpec subDataSpec =
        create_pubsubspec(qemuInboundTopicName, SilKit::Services::MatchingLabel::Kind::Mandatory);

    const PubSubSpec pubDataSpec = create_pubsubspec(qemuOutboundTopicName,
                                                     SilKit::Services::MatchingLabel::Kind::Optional);


    try
    {
        auto participantConfiguration =
            SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);

        QemuSocketTransmitter transmitter{ioContext, qemuHostname, qemuService, publisherName, subscriberName, pubDataSpec,
                                                       subDataSpec, participant.get()};

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

// Copyright (c) Vector Informatik GmbH. All rights reserved.

#pragma once 

#include <string>
#include <set>
#include <asio/ts/net.hpp>
#include "silkit/SilKit.hpp"
#include "silkit/services/pubsub/all.hpp"
#include "../Utility/StringUtils.hpp"
#include "silkit/util/serdes/Deserializer.hpp"
#include "silkit/util/serdes/Serializer.hpp"
#include "adapter/Exceptions.hpp"

namespace asio {
class io_context;
}

namespace adapters {
namespace chardev {

class ChardevSocketToPubSubAdapter
{
    typedef std::string string;
    typedef SilKit::Services::PubSub::PubSubSpec PubSubSpec;

public:
    //the template parameter is here to allow anything which can be converted to asio::buffer
    //If the elements are not byte-wide, the behavior is unspecified. If it even compiles.
    template <class vector_like_container>
    void SendToSocket(const vector_like_container& data);

    ChardevSocketToPubSubAdapter(asio::io_context& io_context, const string& host, const string& service,
                                 const string& publisherName, const string& subscriberName,
                                 const PubSubSpec& pubDataSpec, const PubSubSpec& subDataSpec,
                                 SilKit::IParticipant* participant);

private:
    //internal callback
    void DoReceiveFrameFromSocket();

private:
    asio::ip::tcp::socket _socket;
    SilKit::Services::Logging::ILogger* _logger;
    std::vector<uint8_t> _data_buffer_fromChardev = {};
    std::vector<uint8_t> _data_buffer_fromChardev_extra = {};
    std::vector<uint8_t> _data_buffer_toChardev = {};
    SilKit::Services::PubSub::IDataPublisher* _publisher;
    SilKit::Services::PubSub::IDataSubscriber* _subscriber;
    SilKit::Util::SerDes::Serializer _serializer;
    SilKit::Util::SerDes::Deserializer _deserializer;
};

/// <summary>
/// Parse the complex string given as the argument requesting an adaptation from a socket to a publisher/subscriber.
/// </summary>
/// <param name="chardevSocketTransmitterArg">The argument as input.</param>
/// <param name="alreadyProvidedSockets">output to log requested sockets.</param>
/// <param name="participantName">Used for creating default publish/subscrib/er names if none requested.</param>
/// <param name="ioContext">Passed to constructor of class ChardevSocketToPubSubAdapter.</param>
/// <param name="participant">Passed to constructor of class ChardevSocketToPubSubAdapter.</param>
/// <param name="logger">Used for printing logging info.</param>
/// <returns>A pointer to the created ChardevSocketToPubSubAdapter.</returns>
ChardevSocketToPubSubAdapter* parseChardevSocketArgument(char* chardevSocketTransmitterArg,
                                                         std::set<std::string>& alreadyProvidedSockets,
                                                         const std::string& participantName,
                                                         asio::io_context& ioContext,
                                                         SilKit::IParticipant* participant,
                                                         SilKit::Services::Logging::ILogger* logger);


} // namespace chardev
} // namespace adapters

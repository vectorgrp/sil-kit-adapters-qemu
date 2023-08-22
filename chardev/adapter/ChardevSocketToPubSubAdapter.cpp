// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "ChardevSocketToPubSubAdapter.hpp"
#include "adapter/Exceptions.hpp"
#include "adapter/Parsing.hpp"

#include <asio/ts/buffer.hpp>
#include <asio/ts/io_context.hpp>
#include <type_traits>

#include "silkit/config/all.hpp"
#include "silkit/services/logging/all.hpp"
#include "silkit/util/serdes/Serialization.hpp"

using namespace SilKit::Services::PubSub;
using namespace std::chrono_literals;
using namespace adapters;
using namespace adapters::chardev;

void ChardevSocketToPubSubAdapter::DoReceiveFrameFromSocket()
{
    _socket.async_read_some(
        asio::buffer(_data_buffer_fromChardev, _data_buffer_fromChardev.size()),
        [this](const std::error_code ec, const std::size_t bytes_received) {
            if (ec)
                throw IncompleteReadError{};

            _logger->Debug(
                "QEMU >> SIL Kit: "
                + std::string(reinterpret_cast<const char*>(_data_buffer_fromChardev.data()), bytes_received));

            _serializer.BeginArray(bytes_received);
            auto publish_buffer = _serializer.ReleaseBuffer();
            auto publish_array_size = publish_buffer.size();
            publish_buffer.reserve(publish_buffer.size() + bytes_received);
            publish_buffer.insert(publish_buffer.end(), _data_buffer_fromChardev.begin(),
                                  _data_buffer_fromChardev.begin() + bytes_received);
            _publisher->Publish(std::move(publish_buffer));

            DoReceiveFrameFromSocket();
        });
}

ChardevSocketToPubSubAdapter::ChardevSocketToPubSubAdapter(asio::io_context& io_context, const std::string& host,
                                                           const std::string& service,
                                                           const std::string& publisherName, const std::string& subscriberName,
                                                           const PubSubSpec& pubDataSpec, const PubSubSpec& subDataSpec,
                                                           SilKit::IParticipant* participant)
    : _socket{io_context}
    , _logger{participant->GetLogger()}
    , _publisher{participant->CreateDataPublisher(publisherName, pubDataSpec)}
    , _subscriber{participant->CreateDataSubscriber(
          subscriberName, subDataSpec,
          [&](SilKit::Services::PubSub::IDataSubscriber* subscriber, const DataMessageEvent& dataMessageEvent) {
              _deserializer.Reset(SilKit::Util::ToStdVector(dataMessageEvent.data));
              this->_data_buffer_toChardev.resize(_deserializer.BeginArray());
              for (auto& val : _data_buffer_toChardev)
              {
                  val = _deserializer.Deserialize<uint8_t>(8);
              }
              _logger->Debug("SIL Kit >> QEMU: "
                             + std::string((const char*)_data_buffer_toChardev.data(), _data_buffer_toChardev.size()));
              _socket.write_some(asio::buffer(_data_buffer_toChardev.data(), _data_buffer_toChardev.size()));
          })}
{
    try
    {
        asio::connect(_socket, asio::ip::tcp::resolver{io_context}.resolve(host, service));
    }
    catch (std::exception& e)
    {
        std::ostringstream error_message;
        error_message << e.what() << std::endl;
        error_message << "Error encountered while trying to connect to QEMU with \"" << chardevArg << "\" at \"" 
                      << host << ':' << service << '"';
        throw std::runtime_error(error_message.str());
    }
    _logger->Info("Socket connect success");
    DoReceiveFrameFromSocket();
}

/// <summary>
/// parses topicname for format "[ns::]topicname[~defaultname]"
/// leaves topicname without extra said format after execution.
/// </summary>
/// <param name="topicname">input/output</param>
/// <param name="defaultname">output</param>
/// <param name="ns">output</param>
void extractAndEraseNamespaceAndDefaultnameFrom(std::string& topicname, 
                                   std::string& defaultname,
                                   std::string& ns)
{
    //reject mandatory labels as topic names:
    throwInvalidCliIf(topicname.find_first_of("=") != std::string::npos);

    auto splitTopic = Utils::split(topicname, "~");

    if (splitTopic.size() == 2)
    {
        defaultname = splitTopic[1];
        throwInvalidCliIf(defaultname == "");
        topicname = splitTopic[0];
    }
    else if (splitTopic.size() != 1)
    {
        throw InvalidCli();
    }

    splitTopic = Utils::split(topicname, ":");
    if (splitTopic.size() == 3 && splitTopic[1] == "")
    {
        ns = splitTopic[0];
        throwInvalidCliIf(ns == "");
        topicname = splitTopic[2];
        return;
    }
    else if (splitTopic.size() == 1)
    {
        return ;
    }
    else
    {
        throw InvalidCli();
    }
};

/// <summary>
/// easier to read version from std::count(s.begin(), s.end(), c);
/// </summary>
template<class iterable>
inline typename std::iterator_traits<typename iterable::const_iterator>::difference_type count(const iterable& s, char c)
{
    return std::count(s.begin(), s.end(), c);
};

void extractTopicLabels(const std::vector<std::string>& args, std::vector<std::string>::iterator& arg_iter,
                    SilKit::Services::PubSub::PubSubSpec& dataSpec)
{
    for (; arg_iter != args.end() && (count(*arg_iter, ':') == 1 || count(*arg_iter, '=') == 1); ++arg_iter)
    {
        auto labelKeyValue = Utils::split(*arg_iter, ":=");
        dataSpec.AddLabel(
            labelKeyValue[0], labelKeyValue[1], [&]() -> auto{
                if ((*arg_iter)[labelKeyValue[0].size()] == '=')
                    return SilKit::Services::MatchingLabel::Kind::Mandatory;
                else
                    return SilKit::Services::MatchingLabel::Kind::Optional;
            }());
    }
};

std::string generateSubscriberNameFrom(const std::string& participantName)
{
    static const std::string base = participantName + "_sub_";
    static int count = 1;
    std::ostringstream subscriberName;
    return static_cast<std::ostringstream&>(subscriberName << base << count++).str();
}

std::string generatePublisherNameFrom(const std::string& participantName)
{
    static const std::string base = participantName + "_pub_";
    static int count = 1;
    std::ostringstream publisherName;
    return static_cast<std::ostringstream&>(publisherName << base << count++).str();
}

ChardevSocketToPubSubAdapter* adapters::chardev::parseChardevSocketArgument(
    char* chardevSocketTransmitterArg, std::set<std::string>& alreadyProvidedSockets,
    const std::string& participantName, asio::io_context& ioContext,
    SilKit::IParticipant* participant, SilKit::Services::Logging::ILogger* logger)
{
    ChardevSocketToPubSubAdapter* newAdapter;
    auto args = Utils::split(chardevSocketTransmitterArg, ",");
    auto arg_iter = args.begin();

    //handle <address>:<port>
    assertAdditionalIterator(arg_iter, args);
    throwInvalidCliIf(alreadyProvidedSockets.insert(*arg_iter).second == false);
    auto portAddress = Utils::split(*arg_iter++, ":");
    throwInvalidCliIf(portAddress.size() != 2);
    const auto& address = portAddress[0];
    const auto& port = portAddress[1];

    //handle inbound topic and labels
    assertAdditionalIterator(arg_iter, args);
    std::string subscriberName = "";
    std::string subscriberNamespace = "";
    extractAndEraseNamespaceAndDefaultnameFrom(*arg_iter, subscriberName, subscriberNamespace);
    PubSubSpec subDataSpec(*arg_iter++, SilKit::Util::SerDes::MediaTypeData());
    if (subscriberNamespace != "")
        subDataSpec.AddLabel("Namespace", subscriberNamespace, SilKit::Services::MatchingLabel::Kind::Mandatory);

    extractTopicLabels(args, arg_iter, subDataSpec);

    //handle outbound topic
    assertAdditionalIterator(arg_iter, args);
    std::string publisherName = "";
    std::string publisherNamespace = "";
    extractAndEraseNamespaceAndDefaultnameFrom(*arg_iter, publisherName, publisherNamespace);
    PubSubSpec pubDataSpec(*arg_iter++, SilKit::Util::SerDes::MediaTypeData());
    if (publisherNamespace != "")
        pubDataSpec.AddLabel("Namespace", publisherNamespace, SilKit::Services::MatchingLabel::Kind::Optional);

    extractTopicLabels(args, arg_iter, pubDataSpec);

    throwInvalidCliIf(arg_iter != args.end());

    //generate publisher and subscriber names if not given
    if (subscriberName == "")
        subscriberName = generateSubscriberNameFrom(participantName);
    if (publisherName == "")
        publisherName = generatePublisherNameFrom(participantName);
    newAdapter = new ChardevSocketToPubSubAdapter(ioContext, address, port, publisherName, subscriberName, pubDataSpec,
                                                  subDataSpec, participant);

    logger->Debug("Created Chardev transmitter " + address + ':' + port + " <" + subscriberName + '('
                  + subDataSpec.Topic() + ')' + " >" + publisherName + '(' + pubDataSpec.Topic() + ')');

    return newAdapter;
}

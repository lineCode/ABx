#include "stdafx.h"
#include "Protocol.h"
#include <random>
#include <ctime>
#include "Logger.h"

namespace Client {

Protocol::Protocol() :
    connection_(nullptr),
    checksumEnabled_(false),
    errorCallback_(nullptr),
    protocolErrorCallback_(nullptr)
{
    inputMessage_ = std::make_shared<InputMessage>();
}

Protocol::~Protocol()
{
    Disconnect();
}

void Protocol::Connect(const std::string& host, uint16_t port)
{
    connection_ = std::make_shared<Connection>();
    connection_->SetErrorCallback(std::bind(&Protocol::OnError, shared_from_this(), std::placeholders::_1));
    connection_->Connect(host, port, std::bind(&Protocol::OnConnect, shared_from_this()));
}

void Protocol::Disconnect()
{
    if (connection_)
    {
        connection_->Close();
        connection_.reset();
    }
}

void Protocol::Send(const std::shared_ptr<OutputMessage>& message)
{
    if (checksumEnabled_)
        message->WriteChecksum();
    message->WriteMessageSize();

    if (connection_)
        connection_->Write(message->GetHeaderBuffer(), message->GetSize());

    message->Reset();
}

void Protocol::Receive()
{
    inputMessage_->Reset();

    // first update message header size
    int headerSize = 2; // 2 bytes for message size
    if (checksumEnabled_)
        headerSize += 4; // 4 bytes for checksum
    inputMessage_->SetHeaderSize(static_cast<uint16_t>(headerSize));

    // read the first 2 bytes which contain the message size
    if (connection_)
        connection_->Read(2, std::bind(&Protocol::InternalRecvHeader, shared_from_this(),
            std::placeholders::_1, std::placeholders::_2));
}

void Protocol::InternalRecvHeader(uint8_t* buffer, uint16_t size)
{
    if (!IsConnected())
        return;

    inputMessage_->FillBuffer(buffer, size);
    uint16_t remainingSize = inputMessage_->ReadSize();
#ifdef _LOGGING
    LOG_DEBUG << "size = " << size << ", remaining size = " << remainingSize << std::endl;
#endif

    // read remaining message data
    if (connection_)
        connection_->Read(remainingSize, std::bind(&Protocol::InternalRecvData,
            shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void Protocol::InternalRecvData(uint8_t* buffer, uint16_t size)
{
    if (!IsConnected())
        return;

#ifdef _LOGGING
    LOG_DEBUG << "size = " << size << std::endl;
#endif

    inputMessage_->FillBuffer(buffer, size);

    if (checksumEnabled_ && !inputMessage_->ReadChecksum())
    {
#ifdef _LOGGING
        LOG_ERROR << "Invalid checksum" << std::endl;
#endif
        return;
    }

    OnReceive(inputMessage_);
}

void Protocol::OnError(const asio::error_code& err)
{
#ifdef _LOGGING
    LOG_ERROR << err.value() << " " << err.message();
#endif
    if (errorCallback_)
        errorCallback_(err);
    Disconnect();
}

}
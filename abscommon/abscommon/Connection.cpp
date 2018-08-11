#include "stdafx.h"
#include "Connection.h"
#include "Dispatcher.h"
#include "Logger.h"
#include "Protocol.h"
#include "Scheduler.h"
#include "StringUtils.h"
#include "Service.h"
#include "OutputMessage.h"

namespace Net {

uint32_t ConnectionManager::maxPacketsPerSec = 0;

Connection::~Connection()
{
    CloseSocket();
}

bool Connection::Send(const std::shared_ptr<OutputMessage>& message)
{
    std::lock_guard<std::recursive_mutex> lockClass(lock_);

    if (state_ != State::Open)
        return false;

    bool noPendingWrite = messageQueue_.empty();
    messageQueue_.emplace_back(message);
    if (noPendingWrite)
        InternalSend(message);

    return true;
}

void Connection::InternalSend(std::shared_ptr<OutputMessage> message)
{
    protocol_->OnSendMessage(message);
    try
    {
        writeTimer_.expires_from_now(std::chrono::seconds(Connection::WriteTimeout));
        readTimer_.async_wait(std::bind(&Connection::HandleTimeout,
            std::weak_ptr<Connection>(shared_from_this()), std::placeholders::_1));

        asio::async_write(socket_,
            asio::buffer(message->GetOutputBuffer(), message->GetMessageLength()),
            std::bind(&Connection::OnWriteOperation, shared_from_this(), std::placeholders::_1));
    }
    catch (asio::system_error& e)
    {
        LOG_ERROR << "Network " << e.code() << " " << e.what() << std::endl;
    }
}

void Connection::OnWriteOperation(const asio::error_code& error)
{
    std::lock_guard<std::recursive_mutex> lockClass(lock_);
    writeTimer_.cancel();
    messageQueue_.pop_front();

    if (error)
    {
        messageQueue_.clear();
        Close(true);
        return;
    }

    if (!messageQueue_.empty())
    {
        InternalSend(messageQueue_.front());
    }
    else if (state_ == State::Closed)
    {
        CloseSocket();
    }
}

void Connection::Close(bool force /* = false */)
{
#ifdef DEBUG_NET
    LOG_DEBUG << "Closing connection" << std::endl;
#endif
    ConnectionManager::Instance()->ReleaseConnection(shared_from_this());

    std::lock_guard<std::recursive_mutex> lockClass(lock_);
    if (state_ != State::Open)
        return;
    state_ = State::Closed;
    if (protocol_)
    {
        Asynch::Dispatcher::Instance.Add(
            Asynch::CreateTask(std::bind(&Protocol::Release, protocol_))
        );
    }

    if (messageQueue_.empty() || force)
        CloseSocket();
}

void Connection::Accept(std::shared_ptr<Protocol> protocol)
{
    protocol_ = protocol;
    Asynch::Dispatcher::Instance.Add(
        Asynch::CreateTask(std::bind(&Protocol::OnConnect, protocol))
    );
    Accept();
}

void Connection::Accept()
{
    std::lock_guard<std::recursive_mutex> lockClass(lock_);
    try
    {
        readTimer_.expires_from_now(std::chrono::seconds(Connection::ReadTimeout));
        readTimer_.async_wait(std::bind(&Connection::HandleTimeout,
            std::weak_ptr<Connection>(shared_from_this()), std::placeholders::_1));

        // Read size of packet
        asio::async_read(socket_,
            asio::buffer(msg_.GetBuffer(), NetworkMessage::HeaderLength),
            std::bind(&Connection::ParseHeader, shared_from_this(), std::placeholders::_1));
    }
    catch (asio::system_error& e)
    {
        LOG_ERROR << "Network " << e.code() << " " << e.what() << std::endl;
        Close(true);
    }
}

void Connection::ParseHeader(const asio::error_code& error)
{
    std::lock_guard<std::recursive_mutex> lockClass(lock_);
    readTimer_.cancel();

    if (error)
    {
#ifdef DEBUG_NET
        // Maybe disconnect
        if (error.value() != 995)
            LOG_ERROR << "Network " << error.value() << " " << error.message() << std::endl;
#endif
        Close(true);
        return;
    }
    if (state_ != State::Open)
    {
#ifdef DEBUG_NET
        LOG_WARNING << "Connection not opened" << std::endl;
#endif
        return;
    }

    uint32_t timePassed = std::max<uint32_t>(1, static_cast<uint32_t>(time(nullptr) - timeConnected_) + 1);
    ++packetsSent_;
    static uint32_t maxPackets = ConnectionManager::maxPacketsPerSec;
    if ((maxPackets != 0) && (packetsSent_ / timePassed) > maxPackets)
    {
        LOG_ERROR << Utils::ConvertIPToString(GetIP()) << " disconnected for exceeding packet per second limit." << std::endl;
        Close(true);
        return;
    }

    if (timePassed > 2)
    {
        timeConnected_ = time(nullptr);
        packetsSent_ = 0;
    }

    int32_t size = msg_.GetHeaderSize();
    if (size == 0 || size >= NETWORKMESSAGE_MAXSIZE - 16)
    {
#ifdef DEBUG_NET
        LOG_WARNING << "Invalid message size " << size << std::endl;
#endif
        Close(true);
        return;
    }

    try
    {
        readTimer_.expires_from_now(std::chrono::seconds(Connection::ReadTimeout));
        readTimer_.async_wait(std::bind(&Connection::HandleTimeout,
            std::weak_ptr<Connection>(shared_from_this()), std::placeholders::_1));

        // Read content
        msg_.SetSize(size + NetworkMessage::HeaderLength);
        asio::async_read(socket_,
            asio::buffer(msg_.GetBodyBuffer(), size),
            std::bind(&Connection::ParsePacket, shared_from_this(), std::placeholders::_1));

    }
    catch (asio::system_error& e)
    {
        LOG_ERROR << "Network " << e.code() << " " << e.what() << std::endl;
        Close(true);
    }
}

void Connection::ParsePacket(const asio::error_code& error)
{
    std::lock_guard<std::recursive_mutex> lockClass(lock_);
    readTimer_.cancel();

    if (error)
    {
        Close(true);
        return;
    }
    if (state_ != State::Open)
    {
#ifdef DEBUG_NET
        LOG_WARNING << "Connection not opened" << std::endl;
#endif
        return;
    }

    uint32_t checksum;;
    int32_t len = msg_.GetMessageLength() - msg_.GetReadPos() - NetworkMessage::ChecksumLength;
    if (len > 0)
        checksum = Utils::AdlerChecksum((uint8_t*)(msg_.GetBuffer() + msg_.GetReadPos() +
            NetworkMessage::ChecksumLength), len);
    else
        checksum = 0;
    uint32_t recvChecksum = msg_.Get<uint32_t>();
    if (recvChecksum != checksum)
        // it might not have been the checksum, step back
        msg_.Skip(-NetworkMessage::ChecksumLength);

    if (!receivedFirst_)
    {
        // First message received
        receivedFirst_ = true;

        if (!protocol_)
        {
            // Game protocol has already been created at this point
            protocol_ = servicePort_->MakeProtocol(recvChecksum == checksum, msg_, shared_from_this());
            if (!protocol_)
            {
                Close(true);
                return;
            }

        }
        else
            // Skip protocol ID
            msg_.Skip(1);

        protocol_->OnRecvFirstMessage(msg_);
    }
    else
        // Send the packet to the current protocol
        protocol_->OnRecvMessage(msg_);

    try
    {
        readTimer_.expires_from_now(std::chrono::seconds(Connection::ReadTimeout));
        readTimer_.async_wait(std::bind(&Connection::HandleTimeout,
            std::weak_ptr<Connection>(shared_from_this()), std::placeholders::_1));

        // Wait for the next packet
        asio::async_read(socket_,
            asio::buffer(msg_.GetBuffer(), NetworkMessage::HeaderLength),
            std::bind(&Connection::ParseHeader, shared_from_this(), std::placeholders::_1));
    }
    catch (asio::system_error& e)
    {
        LOG_ERROR << "Network " << e.code() << " " << e.what() << std::endl;
        Close(true);
    }
}

void Connection::HandleTimeout(std::weak_ptr<Connection> weakConn, const asio::error_code& error)
{
    if (error == asio::error::operation_aborted)
        return;

    // It needs a constant stream of packets or the connection will be closed.
    // Send at least a ping once in a while.
#ifdef DEBUG_NET
    LOG_DEBUG << "Timeout, closing connection" << std::endl;
#endif

    if (auto conn = weakConn.lock())
    {
        conn->Close(true);
    }
}

void Connection::CloseSocket()
{
    if (socket_.is_open())
    {
#ifdef DEBUG_NET
        LOG_DEBUG << "Closing socket" << std::endl;
#endif
        try
        {
            readTimer_.cancel();
            writeTimer_.cancel();
            asio::error_code err;
            socket_.shutdown(asio::ip::tcp::socket::shutdown_both, err);
            socket_.close(err);
        }
        catch (asio::system_error& e)
        {
            LOG_ERROR << "Network " << e.code() << " " << e.what() << std::endl;
        }
    }
}

std::shared_ptr<Connection> ConnectionManager::CreateConnection(
    asio::io_service& ioService, std::shared_ptr<ServicePort> servicer)
{
    if (connections_.size() >= SERVER_MAX_CONNECTIONS)
    {
        LOG_ERROR << "To many connections" << std::endl;
        return std::shared_ptr<Connection>();
    }

    std::shared_ptr<Connection> connection = std::make_shared<Connection>(ioService, servicer);
    std::lock_guard<std::mutex> lock(lock_);
    connections_.insert(connection);

    return connection;
}

void ConnectionManager::ReleaseConnection(std::shared_ptr<Connection> connection)
{
#ifdef DEBUG_NET
    LOG_DEBUG << "Releasing connection" << std::endl;
#endif
    std::lock_guard<std::mutex> lockClass(lock_);
    connections_.erase(connection);
}

void ConnectionManager::CloseAll()
{
#ifdef DEBUG_NET
    LOG_DEBUG << "Closing all connections" << std::endl;
#endif

    std::lock_guard<std::mutex> lockClass(lock_);

    for (const auto& conn : connections_)
    {
        try
        {
            asio::error_code err;
            conn->socket_.shutdown(asio::ip::tcp::socket::shutdown_both, err);
            conn->socket_.close(err);
        }
        catch (asio::system_error&) {}
    }
    connections_.clear();
}

uint32_t Connection::GetIP() const
{
    asio::error_code err;
    const asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint(err);
    if (!err)
        return endpoint.address().to_v4().to_uint();
    return 0;
}

uint16_t Connection::GetPort() const
{
    asio::error_code err;
    const asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint(err);
    if (!err)
        return endpoint.port();
    return 0;
}

}

#include "stdafx.h"
#include "Service.h"
#include "Connection.h"
#include "Scheduler.h"
#include "NetworkMessage.h"
#include "Logger.h"
#include <AB/CommonConfig.h>
#include "Subsystems.h"

namespace Net {

uint16_t ServiceManager::GetFreePort()
{
    asio::io_service service;
    asio::ip::tcp::acceptor acceptor(service);
    unsigned short port(0);
    asio::ip::tcp::endpoint endPoint(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
    acceptor.open(endPoint.protocol());
    acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endPoint);
    acceptor.listen();        // NEEDED TO ADD THIS BIT!
    asio::ip::tcp::endpoint le = acceptor.local_endpoint(); //THIS LINE SOLVES IT
    port = le.port();
    acceptor.close();
    return port;
}

void ServiceManager::Run()
{
    assert(!running_);
    running_ = true;
}

void ServiceManager::Stop()
{
    if (!running_)
        return;
    for (const auto& sp : acceptors_)
    {
        ioService_.post(std::bind(&ServicePort::OnStopServer, sp.second));
    }
    acceptors_.clear();
    running_ = false;
}

std::list<std::pair<uint32_t, uint16_t>> ServiceManager::GetPorts() const
{
    std::list<std::pair<uint32_t, uint16_t>> ports;
    for (auto it = acceptors_.begin(); it != acceptors_.end(); ++it)
    {
        ports.push_back(it->first);
    }
    // Maps are ordered, so the elements are in order
    ports.unique();
    return ports;
}

bool ServicePort::Open(uint32_t ip, uint16_t port)
{
    Close();

    serverPort_ = port;
    serverIp_ = ip;
    try
    {
        // No reuse address
        acceptor_.reset(new asio::ip::tcp::acceptor(service_, asio::ip::tcp::endpoint(
            asio::ip::address(asio::ip::address_v4(serverIp_)), serverPort_), false));
#ifdef TCP_OPTION_NODELAY
        acceptor_->set_option(asio::ip::tcp::no_delay(true));
#endif
        Accept();
        return true;
    }
    catch (asio::system_error& e)
    {
        LOG_ERROR << "Network (" << e.code().value() << "): " << e.what() << std::endl;
        return false;
    }
}

void ServicePort::Close()
{
    if (acceptor_)
    {
        if (acceptor_->is_open())
        {
            asio::error_code error;
            acceptor_->close(error);
            if (error)
            {
                LOG_ERROR << "Closing socket " << error.message() << std::endl;
            }
        }
    }
}

bool ServicePort::AddService(std::shared_ptr<ServiceBase> service)
{
    for (ConstIt it = services_.begin(); it != services_.end(); ++it)
    {
        if ((*it)->IsSingleSocket())
            return false;
    }

    services_.push_back(service);
    return true;
}

void ServicePort::OnStopServer()
{
    Close();
}

std::shared_ptr<Protocol> ServicePort::MakeProtocol(bool checksummed,
    NetworkMessage& msg, std::shared_ptr<Connection> connection) const
{
    uint8_t protocolId = msg.GetByte();
    for (ConstIt it = services_.begin(); it != services_.end(); ++it)
    {
        std::shared_ptr<ServiceBase> service = *it;
        if (service->GetPotocolIdentifier() == protocolId &&
            ((checksummed && service->IsChecksummed()) || !service->IsChecksummed()))
        {
            return service->MakeProtocol(connection);
        }
    }
    return std::shared_ptr<Protocol>();
}

void ServicePort::Accept()
{
    if (!acceptor_)
    {
#ifdef DEBUG_NET
        LOG_INFO << "acceptor_ == null" << std::endl;
#endif
        return;
    }

    try
    {
        auto connMan = GetSubsystem<ConnectionManager>();
        if (!connMan)
        {
            LOG_ERROR << "No ConnectionManager subsystem!" << std::endl;
            return;
        }
        std::shared_ptr<Connection> conn = connMan->CreateConnection(
            service_, shared_from_this()
        );

        if (conn)
        {
            acceptor_->async_accept(conn->GetSocket(),
                std::bind(&ServicePort::OnAccept, shared_from_this(), conn, std::placeholders::_1));
        }
    }
    catch (asio::system_error& e)
    {
#ifdef DEBUG_NET
        LOG_ERROR << "Network " << e.what() << std::endl;
#else
        AB_UNUSED(e);
#endif
    }
}

void ServicePort::OnAccept(std::shared_ptr<Connection> connection, const asio::error_code& error)
{
    if (!error)
    {
        if (services_.empty())
            return;

        uint32_t remoteIp = connection->GetIP();
        if (remoteIp != 0 && acceptConnection_(remoteIp))
        {
            if (services_.front()->IsSingleSocket())
            {
                // Only one handler, and it will send first
                connection->Accept(services_.front()->MakeProtocol(connection));
            }
            else
                connection->Accept();
        }
        else
        {
            connection->Close();
        }

#ifdef DEBUG_NET
        LOG_DEBUG << "Accept OK" << std::endl;
#endif
        Accept();
    }
    else if (error != asio::error::operation_aborted)
    {
    }
#ifdef DEBUG_NET
    else
        LOG_ERROR << "Operation aborted" << std::endl;
#endif
}

}

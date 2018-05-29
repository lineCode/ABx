#pragma once

#include "Connection.h"
#include "Logger.h"
#include <abcrypto.hpp>

namespace Net {

class OutputMessage;

class Protocol : public std::enable_shared_from_this<Protocol>
{
protected:
    const std::weak_ptr<Connection> connection_;
    std::shared_ptr<OutputMessage> outputBuffer_;
    bool checksumEnabled_;
    bool encryptionEnabled_;
    void XTEAEncrypt(OutputMessage& msg) const;
    bool XTEADecrypt(NetworkMessage& msg) const;

    void Disconnect() const
    {
        if (auto conn = GetConnection())
            conn->Close();
    }
    virtual void Release() {}

    friend class Connection;
public:
    explicit Protocol(std::shared_ptr<Connection> connection) :
        connection_(connection),
        checksumEnabled_(false),
        encryptionEnabled_(false)
    {
    }
#if defined(_DEBUG)
    virtual ~Protocol()
    {
#ifdef DEBUG_NET
//        LOG_DEBUG << std::endl;
#endif
    }
#else
    virtual ~Protocol() = default;
#endif
    Protocol(const Protocol&) = delete;
    Protocol& operator=(const Protocol&) = delete;

    virtual void OnSendMessage(const std::shared_ptr<OutputMessage>& message) const;
    void OnRecvMessage(NetworkMessage& message);

    virtual void OnRecvFirstMessage(NetworkMessage& msg) = 0;
    virtual void OnConnect() {}

    virtual void ParsePacket(NetworkMessage&) {
    }

    bool IsConnectionExpired() const { return connection_.expired(); }
    std::shared_ptr<Connection> GetConnection() const { return connection_.lock(); }

    std::shared_ptr<OutputMessage> GetOutputBuffer(int32_t size);
    void ResetOutputBuffer();
    uint32_t GetIP()
    {
        if (auto c = GetConnection())
            return c->GetIP();
        return 0;
    }
    std::shared_ptr<OutputMessage>& GetCurrentBuffer()
    {
        return outputBuffer_;
    }

    void Send(std::shared_ptr<OutputMessage> message)
    {
        if (auto conn = GetConnection())
        {
            conn->Send(message);
        }
    }
};

}
#pragma once

#include <AB/Entities/Service.h>
#include "UuidUtils.h"

namespace IO {
class DataClient;
}

namespace Net {
class MessageClient;
}

class ServerApp
{
private:
    void Init();
protected:
    bool running_;
    std::string serverId_;
    AB::Entities::ServiceType serverType_;
    std::string machine_;
    std::string serverName_;
    std::string serverLocation_;
    std::string configFile_;
    std::string logDir_;
    std::string serverHost_;
    std::string serverIp_;
    uint16_t serverPort_;
    /// Get a generic currently unique server name
    std::string GetFreeName(IO::DataClient* client);
    virtual bool ParseCommandLine();
public:
    ServerApp() :
        running_(false),
        serverId_(Utils::Uuid::EMPTY_UUID),
        serverType_(AB::Entities::ServiceTypeUnknown),
        machine_(""),
        serverName_(""),
        serverLocation_(""),
        serverPort_(std::numeric_limits<uint16_t>::max())
    { }
    virtual ~ServerApp() = default;
    bool InitializeW(int argc, wchar_t** argv);
    bool InitializeA(int argc, char** argv);
    virtual bool Initialize(const std::vector<std::string>& args);
    virtual void Run()
    { }
    virtual void Stop()
    { }
    /// Returns the Server UUID from the config file
    const std::string& GetServerId() const
    {
        return serverId_;
    }
    const std::string& GetServerName() const
    {
        return serverName_;
    }
    bool SendServerJoined(Net::MessageClient* client, const AB::Entities::Service& service);
    bool SendServerLeft(Net::MessageClient* client, const AB::Entities::Service& service);
    bool GetCommandLineValue(const std::string& name, std::string& value);
    bool GetCommandLineValue(const std::string& name);

    std::string path_;
    std::string exeFile_;
    std::vector<std::string> arguments_;
};

#pragma once

#include "ServerApp.h"
#include "Server.h"
#include "IpList.h"

class Application : public ServerApp
{
private:
    uint32_t listenIp_;
    size_t maxSize_;
    bool readonly_;
    asio::io_service ioService_;
    std::unique_ptr<Server> server_;
    uint32_t flushInterval_;
    uint32_t cleanInterval_;
    Net::IpList whiteList_;
    bool LoadConfig();
    void PrintServerInfo();
    void ShowHelp();
protected:
    bool ParseCommandLine() override;
public:
    Application();
    ~Application() override;

    bool Initialize(const std::vector<std::string>& args) override;
    void Run() override;
    void Stop() override;
};


#pragma once

#include "ServerApp.h"
#include <AB/Entities/Account.h>
#if __cplusplus < 201703L
#   if !defined(__clang__) && !defined(__GNUC__)
#       include <filesystem>
#   else
#       include <experimental/filesystem>
#   endif
#else
#   include <filesystem>
#endif
#include "MessageClient.h"

#if __cplusplus < 201703L
// C++14
namespace fs = std::experimental::filesystem;
#else
// C++17
namespace fs = std::filesystem;
#endif

using HttpsServer = SimpleWeb::Server<SimpleWeb::HTTPS>;

namespace IO {
class DataClient;
}

template<typename T>
inline size_t stream_size(T& s)
{
    s.seekg(0, std::ios::end);
    size_t size = s.tellg();
    s.seekg(0, std::ios::beg);
    return size;
}

class Application : public ServerApp, public std::enable_shared_from_this<Application>
{
private:
    bool requireAuth_;
    std::shared_ptr<asio::io_service> ioService_;
    int64_t startTime_;
    uint64_t bytesSent_;
    uint32_t uptimeRound_;
    int64_t statusMeasureTime_;
    int64_t lastLoadCalc_;
    bool temporary_;

    std::unique_ptr<HttpsServer> server_;
    std::string root_;
    std::string dataHost_;
    uint16_t dataPort_;
    /// Byte/sec
    uint64_t maxThroughput_;
    std::vector<int> loads_;
    std::mutex mutex_;
    void HandleMessage(const Net::MessageMsg& msg);
    void ShowHelp();
    void UpdateBytesSent(size_t bytes);
    void HeartBeatTask();
    bool IsAllowed(std::shared_ptr<HttpsServer::Request> request);
    static SimpleWeb::CaseInsensitiveMultimap GetDefaultHeader();
    void GetHandlerDefault(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerGames(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerSkills(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerProfessions(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerAttributes(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerEffects(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerItems(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerMusic(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerVersion(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void GetHandlerVersions(std::shared_ptr<HttpsServer::Response> response,
        std::shared_ptr<HttpsServer::Request> request);
    void HandleError(std::shared_ptr<HttpsServer::Request> /*request*/,
        const SimpleWeb::error_code& ec);
    bool HandleOnAccept(const asio::ip::tcp::endpoint& endpoint);

    uint8_t GetAvgLoad() const
    {
        if (loads_.empty())
            return 0;

        float loads = 0.0f;
        for (int p : loads_)
            loads += static_cast<float>(p);
        return static_cast<uint8_t>(loads / loads_.size());
    }
protected:
    bool ParseCommandLine() override;
public:
    Application();
    ~Application() override;

    bool Initialize(const std::vector<std::string>& args) override;
    void Run() override;
    void Stop() override;
    void SpawnServer();
};


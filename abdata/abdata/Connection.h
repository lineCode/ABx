#pragma once

#include <stdint.h>
#include <vector>
#include "StorageProvider.h"
#include "Dispatcher.h"
#include "DataKey.h"
#include "Subsystems.h"
#include "DataCodes.h"

class ConnectionManager;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	explicit Connection(asio::io_service& io_service, ConnectionManager& manager,
        StorageProvider& storage, size_t maxData, uint16_t maxKeySize_);
    asio::ip::tcp::socket& socket();
	void Start();
	void Stop();
private:
    template <typename Callable, typename... Args>
    void AddTask(Callable function, Args&&... args)
    {
        GetSubsystem<Asynch::Dispatcher>()->Add(
            Asynch::CreateTask(std::bind(function, shared_from_this(), std::forward<Args>(args)...))
        );
    }

    void StartCreateOperation();
    void StartUpdateDataOperation();
    void StartReadOperation();
    void StartDeleteOperation();
    void StartInvalidateOperation();
    void StartPreloadOperation();
    void StartExistsOperation();
    void StartClearOperation();

    void HandleUpdateReadRawData(const asio::error_code& error, size_t bytes_transferred, size_t expected);
    void HandleCreateReadRawData(const asio::error_code& error, size_t bytes_transferred, size_t expected);
    void HandleReadReadRawData(const asio::error_code& error, size_t bytes_transferred, size_t expected);
    void HandleWriteReqResponse(const asio::error_code& error);
    void HandleExistsReadRawData(const asio::error_code& error, size_t bytes_transferred, size_t expected);
    void StartClientRequestedOp();
    void StartReadKey(uint16_t& keySize);
    void SendResponseAndStart(std::vector<asio::mutable_buffer>& resp, size_t size);
    void SendStatusAndRestart(IO::ErrorCodes code, const std::string& message);

    static inline uint32_t ToInt32(const std::vector<uint8_t>& intBytes, size_t start)
    {
        return (intBytes[start + 3] << 24) | (intBytes[start + 2] << 16) | (intBytes[start + 1] << 8) | intBytes[start];
    }
    static inline uint16_t ToInt16(const std::vector<uint8_t>& intBytes, size_t start)
    {
        return  (intBytes[start + 1] << 8) | intBytes[start];
    }

    size_t maxDataSize_;
    uint16_t maxKeySize_;
    asio::ip::tcp::socket socket_;
    ConnectionManager& connectionManager_;
    StorageProvider& storageProvider_;
    IO::OpCodes opcode_;

    IO::DataKey key_;
    std::shared_ptr<std::vector<uint8_t>> data_;
};

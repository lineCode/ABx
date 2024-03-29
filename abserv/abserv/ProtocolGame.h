#pragma once

#include "Protocol.h"
#include "Connection.h"
#include "Dispatcher.h"
#include <AB/ProtocolCodes.h>
#include "Subsystems.h"
#include "InputQueue.h"

namespace Game {
class Player;
}

namespace Net {

class ProtocolGame final : public Protocol
{
public:
    // static protocol information
    enum { ServerSendsFirst = true };
    enum { ProtocolIdentifier = 0 }; // Not required as we send first
    enum { UseChecksum = true };
    static const char* ProtocolName() { return "Game Protocol"; };
    friend class Game::Player;
    static std::string serverId_;
private:
    std::weak_ptr<Game::Player> player_;
    uint32_t challengeTimestamp_ = 0;
    uint8_t challengeRandom_ = 0;
    DH_KEY clientKey_;
    std::shared_ptr<Game::Player> GetPlayer()
    {
        return player_.lock();
    }
    void AddPlayerInput(Game::InputType type, const Utils::VariantMap& data);
    void AddPlayerInput(Game::InputType type);
public:
    explicit ProtocolGame(std::shared_ptr<Connection> connection) :
        Protocol(connection)
    {
        checksumEnabled_ = ProtocolGame::UseChecksum;
        compressionEnabled_ = ENABLE_GAME_COMPRESSION;
        encryptionEnabled_ = ENABLE_GAME_ENCRYTION;
        // TODO:
        SetEncKey(AB::ENC_KEY);
    }

    void Login(const std::string& playerUuid, const uuids::uuid& accountUuid,
        const std::string& mapUuid, const std::string& instanceUuid);
    void Logout();
    /// The client requests to enter a game. Find/create it, add the player and return success.
    void EnterGame();
    /// Tells the client to change the instance. The client will disconnect and reconnect to enter the instance.
    void ChangeInstance(const std::string& mapUuid, const std::string& instanceUuid);
    void WriteToOutput(const NetworkMessage& message);
private:
    template <typename Callable, typename... Args>
    void AddPlayerTask(Callable function, Args&&... args)
    {
        auto player = GetPlayer();
        if (!player)
            return;
        GetSubsystem<Asynch::Dispatcher>()->Add(
            Asynch::CreateTask(std::bind(function, player, std::forward<Args>(args)...))
        );
    }

    std::shared_ptr<ProtocolGame> GetThis()
    {
        return std::static_pointer_cast<ProtocolGame>(shared_from_this());
    }
    void ParsePacket(NetworkMessage& message) final;
    void OnRecvFirstMessage(NetworkMessage& msg) final;
    void OnConnect() final;

    void DisconnectClient(uint8_t error);
    void Connect();

    bool acceptPackets_ = false;
};

}

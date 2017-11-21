#pragma once

#include <memory>
#include "Protocol.h"
#include "Connection.h"
#include "Dispatcher.h"

namespace Game {
class Player;
}

namespace Net {

enum PacketType : uint8_t
{
    PacketTypeLogout = 0x14,

    PacketTypeMoveNorth = 0x65,
    PacketTypeMoveNorthEast = 0x66,
    PacketTypeMoveEast = 0x67,
    PacketTypeMoveSouthEast = 0x68,
    PacketTypeMoveSouth = 0x69,
    PacketTypeMoveSouthWest = 0x70,
    PacketTypeMoveWest = 0x71,
    PacketTypeMoveNorthWest = 0x72,
};


class ProtocolGame final : public Protocol
{
public:
    // static protocol information
    enum { ServerSendsFirst = true };
    enum { ProtocolIdentifier = 0 }; // Not required as we send first
    enum { UseChecksum = true };
    static const char* ProtocolName() { return "Game Protocol"; };
    friend class Game::Player;
private:
    std::shared_ptr<Game::Player> player_;
public:
    explicit ProtocolGame(std::shared_ptr<Connection> connection) :
        Protocol(connection)
    {
        checksumEnabled_ = ProtocolGame::UseChecksum;
    }

    void Login(const std::string& name, uint32_t accountId);
    void Logout();
private:
    // Helpers so we don't need to bind every time
    template <typename Callable, typename... Args>
    void AddGameTask(Callable function, Args&&... args)
    {
        Asynch::Dispatcher::Instance.Add(
            Asynch::CreateTask(std::bind(function, player_->GetGame(), std::forward<Args>(args)...))
        );
    }

    template <typename Callable, typename... Args>
    void AddGameTaskTimed(uint32_t delay, Callable function, Args&&... args)
    {
        Asynch::Dispatcher::Instance.Add(
            Asynch::CreateTask(delay, std::bind(function, player_->GetGame(), std::forward<Args>(args)...))
        );
    }

    std::shared_ptr<ProtocolGame> GetThis()
    {
        return std::static_pointer_cast<ProtocolGame>(shared_from_this());
    }
    void ParsePacket(NetworkMessage& message) final;
    void OnRecvFirstMessage(NetworkMessage& msg) final;
    void OnConnect() final;

    void DisconnectClient(const std::string& message);
    void Connect(uint32_t playerId);

    bool acceptPackets_ = false;
};

}

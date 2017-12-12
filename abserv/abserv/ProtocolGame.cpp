#include "stdafx.h"
#include "ProtocolGame.h"
#include "Player.h"
#include "PlayerManager.h"
#include "OutputMessage.h"
#include "IOPlayer.h"
#include "IOAccount.h"
#include "Bans.h"
#include "Utils.h"
#include "GameManager.h"
#include "Logger.h"
#include "Game.h"
#include "Random.h"
#include <AB/ProtocolCodes.h>

#include "DebugNew.h"

namespace Net {

void ProtocolGame::Login(const std::string& name, uint32_t accountId, const std::string& map)
{
    std::shared_ptr<Game::Player> foundPlayer = Game::PlayerManager::Instance.GetPlayerByName(name);
    if (foundPlayer)
    {
        DisconnectClient(AB::Errors::AlreadyLoggedIn);
        return;
    }

    player_ = Game::PlayerManager::Instance.CreatePlayer(name, GetThis());
    if (!DB::IOPlayer::PreloadPlayer(player_.get(), name))
    {
        DisconnectClient(AB::Errors::ErrorLoadingCharacter);
        return;
    }

    if (Auth::BanManager::Instance.IsAccountBanned(accountId))
    {
        DisconnectClient(AB::Errors::AccountBanned);
        return;
    }

    if (!DB::IOPlayer::LoadPlayerByName(player_.get(), name))
    {
        DisconnectClient(AB::Errors::ErrorLoadingCharacter);
        return;
    }

    player_->map_ = map;
    Connect(player_->id_);
    OutputMessagePool::Instance()->AddToAutoSend(shared_from_this());
}

void ProtocolGame::Logout()
{
    if (!player_)
        return;

    player_->logoutTime_ = Utils::AbTick();
    DB::IOPlayer::SavePlayer(player_.get());
    Game::PlayerManager::Instance.RemovePlayer(player_->id_);
    Disconnect();
    OutputMessagePool::Instance()->RemoveFromAutoSend(shared_from_this());
    player_.reset();
}

void ProtocolGame::ParsePacket(NetworkMessage& message)
{
    if (!acceptPackets_ ||
        Game::GameManager::Instance.GetState() != Game::GameManager::Running ||
        message.GetSize() == 0)
        return;

    uint8_t recvByte = message.GetByte();

    switch (recvByte)
    {
    case AB::GameProtocol::PacketTypePing:
        AddGameTask(&Game::Game::Ping, player_->id_);
        break;
    case AB::GameProtocol::PacketTypeLogout:
        AddPlayerTask(&Game::Player::Logout);
        break;
    case AB::GameProtocol::PacketTypeMoveNorth:
        AddPlayerTask(&Game::Player::PlayerMove, Game::MoveDirectionNorth);
        break;
    case AB::GameProtocol::PacketTypeMoveNorthEast:
        AddPlayerTask(&Game::Player::PlayerMove, Game::MoveDirectionNorthEast);
        break;
    case AB::GameProtocol::PacketTypeMoveEast:
        AddPlayerTask(&Game::Player::PlayerMove, Game::MoveDirectionEast);
        break;
    case AB::GameProtocol::PacketTypeMoveSouthEast:
        AddPlayerTask(&Game::Player::PlayerMove, Game::MoveDirectionNorth);
        break;
    case AB::GameProtocol::PacketTypeMoveSouth:
        AddPlayerTask(&Game::Player::PlayerMove, Game::MoveDirectionSouth);
        break;
    case AB::GameProtocol::PacketTypeMoveSouthWest:
        AddPlayerTask(&Game::Player::PlayerMove, Game::MoveDirectionSouthWest);
        break;
    case AB::GameProtocol::PacketTypeMoveWest:
        AddPlayerTask(&Game::Player::PlayerMove, Game::MoveDirectionWest);
        break;
    case AB::GameProtocol::PacketTypeMoveNorthWest:
        AddPlayerTask(&Game::Player::PlayerMove, Game::MoveDirectionNorthWest);
        break;
    case AB::GameProtocol::PacketTypeUseSkill:
        break;
    case AB::GameProtocol::PacketTypeCancelSkill:
        break;
    case AB::GameProtocol::PacketTypeAttack:
        break;
    case AB::GameProtocol::PacketTypeCancelAttack:
        break;
    default:
        LOG_INFO << "Player " << player_->GetName() << " sent an unknown packet header: 0x" <<
            std::hex << static_cast<uint16_t>(recvByte) << std::dec << std::endl;
        break;
    }
}

void ProtocolGame::OnRecvFirstMessage(NetworkMessage& msg)
{
    msg.Skip(2);    // Client OS
    uint16_t version = msg.Get<uint16_t>();
    if (version != AB::PROTOCOL_VERSION)
    {
        DisconnectClient(AB::Errors::WrongProtocolVersion);
        return;
    }
    std::string accountName = msg.GetString();
    if (accountName.empty())
    {
        DisconnectClient(AB::Errors::InvalidAccountName);
        return;
    }
    std::string password = msg.GetString();
    std::string characterName = msg.GetString();
    std::string map = msg.GetString();

    if (Auth::BanManager::Instance.IsIpBanned(GetIP()))
    {
        DisconnectClient(AB::Errors::IPBanned);
        return;
    }

    uint32_t accountId = DB::IOAccount::GameWorldAuth(accountName, password, characterName);
    if (accountId == 0)
    {
        DisconnectClient(AB::Errors::NamePasswordMismatch);
        return;
    }

    Asynch::Dispatcher::Instance.Add(
        Asynch::CreateTask(
            std::bind(&ProtocolGame::Login, GetThis(), characterName, accountId, map)
        )
    );
}

void ProtocolGame::OnConnect()
{
/*    std::shared_ptr<OutputMessage> output = OutputMessagePool::Instance()->GetOutputMessage();

    // Skip checksum
//    output->Skip(sizeof(uint32_t));

    // Packet length & type
    output->Add<uint16_t>(0x0006);
    output->AddByte(0x1F);

    // Add timestamp & random number
    challengeTimestamp_ = static_cast<uint32_t>(time(nullptr));
    output->Add<uint32_t>(challengeTimestamp_);

    challengeRandom_ = Utils::Random::Instance.Get<uint8_t>();
    output->AddByte(challengeRandom_);

    Send(output);
    */
}

void ProtocolGame::DisconnectClient(uint8_t error)
{
    std::shared_ptr<OutputMessage> output = OutputMessagePool::Instance()->GetOutputMessage();
    output->AddByte(AB::GameProtocol::Error);
    output->AddByte(error);
    Send(output);
    Disconnect();
}

void ProtocolGame::Connect(uint32_t playerId)
{
    if (IsConnectionExpired())
        // ProtocolGame::release() has been called at this point and the Connection object
        // no longer exists, so we return to prevent leakage of the Player.
        return;

    std::shared_ptr<Game::Player> foundPlayer = Game::PlayerManager::Instance.GetPlayerById(playerId);
    if (!foundPlayer)
    {
        DisconnectClient(AB::Errors::AlreadyLoggedIn);
        return;
    }

    player_ = foundPlayer;
    player_->loginTime_ = Utils::AbTick();

    acceptPackets_ = true;

    Asynch::Dispatcher::Instance.Add(
        Asynch::CreateTask(
            std::bind(&ProtocolGame::EnterGame, GetThis(), player_->map_)
        )
    );
}

void ProtocolGame::WriteToOutput(const NetworkMessage& message)
{
    std::shared_ptr<Net::OutputMessage> po = GetOutputBuffer(message.GetSize());
    po->Append(message);
}

void ProtocolGame::EnterGame(const std::string& mapName)
{
    if (Game::GameManager::Instance.AddPlayer(mapName, player_))
    {
        std::shared_ptr<OutputMessage> output = OutputMessagePool::Instance()->GetOutputMessage();
        output->AddByte(AB::GameProtocol::GameEnter);
        output->AddString(mapName);
        // Object ID in game
        output->Add<uint32_t>(player_->id_);
        Send(output);
    }
    else
        DisconnectClient(AB::Errors::CannotEnterGame);
}

}

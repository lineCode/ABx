#include "stdafx.h"
#include "ProtocolGame.h"
#include "Player.h"
#include "PlayerManager.h"
#include "OutputMessage.h"
#include "IOPlayer.h"
#include "IOAccount.h"
#include "Bans.h"
#include "StringUtils.h"
#include "GameManager.h"
#include "Logger.h"
#include "Game.h"
#include "Random.h"
#include "Variant.h"
#include "IOGame.h"
#include "stdafx.h"
#include "ConfigManager.h"

#include "DebugNew.h"

namespace Net {

void ProtocolGame::Login(const std::string& playerUuid, const uuids::uuid& accountUuid,
    const std::string& mapUuid)
{
    std::shared_ptr<Game::Player> foundPlayer = Game::PlayerManager::Instance.GetPlayerByUuid(playerUuid);
    if (foundPlayer)
    {
        DisconnectClient(AB::Errors::AlreadyLoggedIn);
        return;
    }

    player_ = Game::PlayerManager::Instance.CreatePlayer(playerUuid, GetThis());

    if (Auth::BanManager::Instance.IsAccountBanned(accountUuid))
    {
        DisconnectClient(AB::Errors::AccountBanned);
        return;
    }

    if (!IO::IOPlayer::LoadPlayerByUuid(player_.get(), playerUuid))
    {
        DisconnectClient(AB::Errors::ErrorLoadingCharacter);
        return;
    }

    IO::DataClient* client = Application::Instance->GetDataClient();
    AB::Entities::Account acc;
    acc.uuid = player_->data_.accountUuid;
    if (!client->Read(acc))
    {
        DisconnectClient(AB::Errors::InvalidAccount);
        return;
    }

    // Check if game exists.
    AB::Entities::Game g;
    g.uuid = mapUuid;
    if (!client->Read(g))
    {
        DisconnectClient(AB::Errors::InvalidGame);
        return;
    }

    acc.onlineStatus = AB::Entities::OnlineStatus::OnlineStatusOnline;
    acc.currentCharacterUuid = player_->data_.uuid;
    acc.currentServerUuid = ConfigManager::Instance[ConfigManager::ServerID].GetString();
    client->Update(acc);

    player_->data_.currentMapUuid = mapUuid;
    client->Update(player_->data_);
    Connect(player_->id_);
    OutputMessagePool::Instance()->AddToAutoSend(shared_from_this());
}

void ProtocolGame::Logout()
{
    if (!player_)
        return;

    player_->logoutTime_ = Utils::AbTick();
    IO::IOPlayer::SavePlayer(player_.get());
    IO::IOAccount::AccountLogout(player_->data_.accountUuid);
    Game::PlayerManager::Instance.RemovePlayer(player_->id_);
    Disconnect();
    OutputMessagePool::Instance()->RemoveFromAutoSend(shared_from_this());
    player_.reset();
}

void ProtocolGame::ParsePacket(NetworkMessage& message)
{
    if (!acceptPackets_ ||
        Game::GameManager::Instance.GetState() != Game::GameManager::ManagerStateRunning ||
        message.GetSize() == 0)
        return;

    uint8_t recvByte = message.GetByte();

    switch (recvByte)
    {
    case AB::GameProtocol::PacketTypePing:
        AddPlayerTask(&Game::Player::Ping);
        // Also update mailbox
        AddPlayerTask(&Game::Player::UpdateMailBox);
        break;
    case AB::GameProtocol::PacketTypeLogout:
        AddPlayerTask(&Game::Player::Logout);
        break;
    case AB::GameProtocol::PacketTypeGetMailHeaders:
        AddPlayerTask(&Game::Player::GetMailHeaders);
        break;
    case AB::GameProtocol::PacketTypeGetMail:
    {
        const std::string mailUuid = message.GetString();
        AddPlayerTask(&Game::Player::GetMail, mailUuid);
        break;
    }
    case AB::GameProtocol::PacketTypeDeleteMail:
    {
        const std::string mailUuid = message.GetString();
        AddPlayerTask(&Game::Player::DeleteMail, mailUuid);
        break;
    }
    case AB::GameProtocol::PacketTypeMove:
    {
        Utils::VariantMap data;
        data[Game::InputDataDirection] = message.Get<uint8_t>();
        player_->inputs_.Add(Game::InputTypeMove, data);
        break;
    }
    case AB::GameProtocol::PacketTypeTurn:
    {
        Utils::VariantMap data;
        data[Game::InputDataDirection] = message.Get<uint8_t>();   // None | Left | Right
        player_->inputs_.Add(Game::InputTypeTurn, data);
        break;
    }
    case AB::GameProtocol::PacketTypeSetDirection:
    {
        Utils::VariantMap data;
        data[Game::InputDataDirection] = message.Get<float>();   // World angle Rad
        player_->inputs_.Add(Game::InputTypeDirection, data);
        break;
    }
    case AB::GameProtocol::PacketTypeGoto:
    {
        Utils::VariantMap data;
        data[Game::InputDataVertexX] = message.Get<float>();
        data[Game::InputDataVertexY] = message.Get<float>();
        data[Game::InputDataVertexZ] = message.Get<float>();
        player_->inputs_.Add(Game::InputTypeGoto, data);
        break;
    }
    case AB::GameProtocol::PacketTypeFollow:
    {
        Utils::VariantMap data;
        data[Game::InputDataObjectId] = message.Get<uint8_t>();
        player_->inputs_.Add(Game::InputTypeFollow, data);
        break;
    }
    case AB::GameProtocol::PacketTypeUseSkill:
    {
        Utils::VariantMap data;
        uint8_t index = message.Get<uint8_t>();
        if (index < PLAYER_MAX_SKILLS)
        {
            data[Game::InputDataSkillIndex] = message.Get<uint8_t>();
            player_->inputs_.Add(Game::InputTypeUseSkill, data);
        }
        break;
    }
    case AB::GameProtocol::PacketTypeCancelSkill:
    {
        Utils::VariantMap data;
        player_->inputs_.Add(Game::InputTypeCancelSkill, data);
        break;
    }
    case AB::GameProtocol::PacketTypeAttack:
    {
        Utils::VariantMap data;
        player_->inputs_.Add(Game::InputTypeAttack, data);
        break;
    }
    case AB::GameProtocol::PacketTypeCancelAttack:
    {
        Utils::VariantMap data;
        player_->inputs_.Add(Game::InputTypeCancelAttack, data);
        break;
    }
    case AB::GameProtocol::PacketTypeSelect:
    {
        Utils::VariantMap data;
        data[Game::InputDataObjectId] = message.Get<uint32_t>();    // Source
        data[Game::InputDataObjectId2] = message.Get<uint32_t>();   // Target
        player_->inputs_.Add(Game::InputTypeSelect, data);
        break;
    }
    case AB::GameProtocol::PacketTypeCommand:
    {
        Utils::VariantMap data;
        data[Game::InputDataCommandType] = message.GetByte();
        data[Game::InputDataCommandData] = message.GetString();
        player_->inputs_.Add(Game::InputTypeCommand, data);
        break;
    }
    default:
        LOG_ERROR << Utils::ConvertIPToString(GetIP()) << ": Player " << player_->GetName() <<
            " sent an unknown packet header: 0x" <<
            std::hex << static_cast<uint16_t>(recvByte) << std::dec << std::endl;
        break;
    }
}

void ProtocolGame::OnRecvFirstMessage(NetworkMessage& msg)
{
    if (encryptionEnabled_)
    {
        if (!XTEADecrypt(msg))
        {
            Disconnect();
            return;
        }
    }

    msg.Skip(2);    // Client OS
    uint16_t version = msg.Get<uint16_t>();
    if (version != AB::PROTOCOL_VERSION)
    {
        DisconnectClient(AB::Errors::WrongProtocolVersion);
        return;
    }
    const std::string accountName = msg.GetString();
    if (accountName.empty())
    {
        DisconnectClient(AB::Errors::InvalidAccountName);
        return;
    }
    const std::string password = msg.GetString();
    const std::string characterName = msg.GetString();
    const std::string map = msg.GetString();

    if (Auth::BanManager::Instance.IsIpBanned(GetIP()))
    {
        DisconnectClient(AB::Errors::IPBanned);
        return;
    }

    const uuids::uuid accountId = IO::IOAccount::GameWorldAuth(accountName, password, characterName);
    if (accountId.nil())
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
            std::bind(&ProtocolGame::EnterGame, GetThis())
        )
    );
}

void ProtocolGame::WriteToOutput(const NetworkMessage& message)
{
    GetOutputBuffer(message.GetSize())->Append(message);
}

void ProtocolGame::EnterGame()
{
    if (Game::GameManager::Instance.AddPlayer(player_->data_.currentMapUuid, player_))
    {
        std::shared_ptr<OutputMessage> output = OutputMessagePool::Instance()->GetOutputMessage();
        output->AddByte(AB::GameProtocol::GameEnter);
        output->AddString(player_->data_.currentMapUuid);
        output->Add<uint32_t>(player_->id_);
        Send(output);
    }
    else
        DisconnectClient(AB::Errors::CannotEnterGame);
}

}

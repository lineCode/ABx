#pragma once

namespace Game {

class Game;
class Player;

enum ChatType : uint8_t
{
    ChannelGuild = 0x01,     // ID = StringHash(Guild.uuid)
    // Local map chat
    ChannelMap = 0x02,       // ID = GameID
    ChannelTrade = 0x03,     // ID = GameID
    // There may be allies on the map that do not belong to the party
    ChannelAllies = 0x04,    //
    ChannelParty = 0x05,     // ID = PartyID
    ChannelWhisper = 0x06,   // ID = PlayerID
};

class ChatChannel
{
protected:
    uint64_t id_;
public:
    ChatChannel(uint64_t id) :
        id_(id)
    {}
    virtual ~ChatChannel() = default;
    virtual bool Talk(Player* player, const std::string& text) {
        AB_UNUSED(player);
        AB_UNUSED(text);
        return false;
    }
};

class GameChatChannel : public ChatChannel
{
private:
    std::weak_ptr<Game> game_;
public:
    GameChatChannel(uint64_t id);
    bool Talk(Player* player, const std::string& text) override;
};

class WhisperChatChannel : public ChatChannel
{
private:
    /// The recipient
    std::weak_ptr<Player> player_;
    std::string playerUuid_;
public:
    WhisperChatChannel(uint64_t id);
    WhisperChatChannel(const std::string& playerUuid);
    bool Talk(Player* player, const std::string& text) override;
    bool Talk(const std::string& playerName, const std::string& text);
};

class GuildChatChannel : public ChatChannel
{
private:
    std::string guildUuid_;
public:
    GuildChatChannel(const std::string& guildUuid);
    bool Talk(Player* player, const std::string& text) override;
    void Broadcast(const std::string& playerName, const std::string& text);
};

class Chat
{
private:
    // Type | ID
    std::map<std::pair<uint8_t, uint64_t>, std::shared_ptr<ChatChannel>> channels_;
public:
    Chat() = default;
    ~Chat() = default;
    // non-copyable
    Chat(const Chat&) = delete;
    Chat& operator=(const Chat&) = delete;

    std::shared_ptr<ChatChannel> Get(uint8_t type, uint64_t id);
    std::shared_ptr<ChatChannel> Get(uint8_t type, const std::string uuid);
    void Remove(uint8_t type, uint64_t id);
    void CleanChats();

    static Chat Instance;
};

}

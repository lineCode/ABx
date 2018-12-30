#pragma once

#include "NetworkMessage.h"
#include "IdGenerator.h"
#include <AB/Entities/Party.h>

namespace Game {

class Player;
class Actor;
class PartyChatChannel;

class Party : public std::enable_shared_from_this<Party>
{
private:
    static Utils::IdGenerator<uint32_t> partyIds_;
    std::weak_ptr<Player> leader_;
    std::array<std::weak_ptr<Player>, AB::Entities::Limits::MAX_PARTY_MEMBERS> members_;
    /// Used when forming a group. If the player accepts it is added to the members.
    std::vector<std::weak_ptr<Player>> invited_;
    std::shared_ptr<PartyChatChannel> chatChannel_;
    /// Depends on the map
    uint32_t maxMembers_;
    size_t numMembers_;
public:
    static uint32_t GetNewId()
    {
        return partyIds_.Next();
    }

    explicit Party(std::shared_ptr<Player> leader);
    Party() = delete;
    // non-copyable
    Party(const Party&) = delete;
    Party& operator=(const Party&) = delete;

    ~Party();

    bool Add(std::shared_ptr<Player> player);
    bool Remove(std::shared_ptr<Player> player);
    bool Invite(std::shared_ptr<Player> player);
    bool RemoveInvite(std::shared_ptr<Player> player);
    void ClearInvites();

    void WriteToMembers(const Net::NetworkMessage& message);

    void SetPartySize(size_t size);
    size_t GetMemberCount() const
    {
        return numMembers_;
    }
    const std::array<std::weak_ptr<Player>, AB::Entities::Limits::MAX_PARTY_MEMBERS>& GetMembers() const
    {
        return members_;
    }
    bool IsFull() const { return numMembers_ >= maxMembers_; }
    bool IsMember(std::shared_ptr<Player> player) const;
    bool IsInvited(std::shared_ptr<Player> player) const;
    bool IsLeader(Player* player);
    std::shared_ptr<Player> GetLeader() const
    {
        return leader_.lock();
    }
    /// Get position of actor in party, 1-based, 0 = not found
    uint8_t GetPosition(const Actor* actor) const;
    /// Tells all members to change the instance. The client will disconnect and reconnect to enter the instance.
    void ChangeInstance(const std::string& mapUuid);

    uint32_t id_;
    AB::Entities::Party data_;
};

}

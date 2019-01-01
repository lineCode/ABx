#pragma once

#include "Party.h"

namespace Game {

class PartyManager
{
private:
    /// The owner of Parties
    std::map<std::string, std::shared_ptr<Party>> parties_;
public:
    PartyManager() = default;
    ~PartyManager()
    {
        parties_.clear();
    }
    std::shared_ptr<Party> GetParty(const std::string& uuid);
    std::shared_ptr<Party> GetPartyById(uint32_t partyId);
    void CleanParties();
};

}
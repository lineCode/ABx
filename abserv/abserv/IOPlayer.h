#pragma once

#include "Player.h"
#include <AB/Entities/Character.h>

namespace IO {

class IOPlayer
{
private:
    static bool LoadPlayer(Game::Player* player);
    static bool LoadPlayerInventory(Game::Player* player);
    static bool SavePlayerInventory(Game::Player* player);
public:
    IOPlayer() = delete;

    static bool LoadCharacter(AB::Entities::Character& ch);
    static bool LoadPlayerByName(Game::Player* player, const std::string& name);
    static bool LoadPlayerByUuid(Game::Player* player, const std::string& uuid);
    static bool SavePlayer(Game::Player* player);
};

}

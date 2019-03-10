#pragma once

#include <AB/Entities/Entity.h>
#include <AB/Entities/Limits.h>

namespace AB {
namespace Entities {

enum StoragePlace : uint8_t
{
    StoragePlaceScene = 0,      // On ground
    StoragePlaceInventory,
    StoragePlaceChest,
    StoragePlaceEquipped
};

static constexpr auto KEY_CONCRETE_ITEMS = "concrete_items";

struct ConcreteItem : Entity
{
    static constexpr const char* KEY()
    {
        return KEY_CONCRETE_ITEMS;
    }
    template<typename S>
    void serialize(S& s)
    {
        s.ext(*this, BaseClass<Entity>{});
        s.text1b(playerUuid, Limits::MAX_UUID);
        s.text1b(accountUuid, Limits::MAX_UUID);
        s.text1b(upgrade1Uuid, Limits::MAX_UUID);
        s.text1b(upgrade2Uuid, Limits::MAX_UUID);
        s.text1b(upgrade3Uuid, Limits::MAX_UUID);
        s.text1b(itemUuid, Limits::MAX_UUID);

        s.value1b(storagePlace);
        s.value2b(storagePos);
        s.value2b(count);
        s.value8b(creation);
    }

    std::string playerUuid = EMPTY_GUID;
    std::string accountUuid = EMPTY_GUID;
    StoragePlace storagePlace = StoragePlaceScene;
    uint16_t storagePos = 0;
    std::string upgrade1Uuid = EMPTY_GUID;
    std::string upgrade2Uuid = EMPTY_GUID;
    std::string upgrade3Uuid = EMPTY_GUID;
    std::string itemUuid = EMPTY_GUID;
    uint16_t count;
    int64_t creation = 0;
};

}
}
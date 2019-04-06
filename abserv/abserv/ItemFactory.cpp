#include "stdafx.h"
#include "ItemFactory.h"
#include "Subsystems.h"
#include "DataClient.h"
#include <AB/Entities/Item.h>
#include <AB/Entities/ConcreteItem.h>
#include "Mechanic.h"
#include <AB/Entities/ItemChanceList.h>
#include "Random.h"

namespace Game {

ItemFactory::ItemFactory() = default;

std::unique_ptr<Item> ItemFactory::CreateItem(const std::string& itemUuid,
    uint32_t level /* = LEVEL_CAP */,
    const std::string& accUuid /* = Utils::Uuid::EMPTY_UUID */,
    const std::string& playerUuid /* = Utils::Uuid::EMPTY_UUID */)
{
    auto client = GetSubsystem<IO::DataClient>();
    AB::Entities::Item gameItem;
    gameItem.uuid = itemUuid;
    if (!client->Read(gameItem))
    {
        LOG_ERROR << "Unable to read item with UUID " << itemUuid << std::endl;
        return std::unique_ptr<Item>();
    }

    std::unique_ptr<Item> result = std::make_unique<Item>(gameItem);
    if (!result->LoadScript(result->data_.script))
        return std::unique_ptr<Item>();

    const uuids::uuid guid = uuids::uuid_system_generator{}();
    AB::Entities::ConcreteItem ci;
    ci.uuid = guid.to_string();
    ci.itemUuid = gameItem.uuid;
    ci.accountUuid = accUuid;
    ci.playerUuid = playerUuid;
    ci.creation = Utils::Tick();
    if (!client->Create(ci))
    {
        LOG_ERROR << "Unable top create concrete item" << std::endl;
        return std::unique_ptr<Item>();
    }
    // Create item stats for this drop
    if (!result->GenerateConcrete(ci, level))
        return std::unique_ptr<Item>();
    // Save the created stats
    client->Update(result->concreteItem_);
    return result;
}

std::unique_ptr<Item> ItemFactory::LoadConcrete(const std::string& concreteUuid)
{
    AB::Entities::ConcreteItem ci;
    auto client = GetSubsystem<IO::DataClient>();
    ci.uuid = concreteUuid;
    if (!client->Read(ci))
    {
        LOG_ERROR << "Error loading concrete item " << concreteUuid << std::endl;
        return std::unique_ptr<Item>();
    }
    AB::Entities::Item gameItem;
    gameItem.uuid = ci.itemUuid;
    if (!client->Read(gameItem))
    {
        LOG_ERROR << "Error loading item " << ci.itemUuid << std::endl;
        return std::unique_ptr<Item>();
    }
    std::unique_ptr<Item> result = std::make_unique<Item>(gameItem);
    if (!result->LoadConcrete(ci))
        return std::unique_ptr<Item>();
    if (!result->LoadScript(result->data_.script))
        return std::unique_ptr<Item>();

    return result;
}

void ItemFactory::DeleteConcrete(const std::string& uuid)
{
    AB::Entities::ConcreteItem ci;
    auto client = GetSubsystem<IO::DataClient>();
    ci.uuid = uuid;
    if (!client->Delete(ci))
    {
        LOG_WARNING << "Error deleting concrete item " << uuid << std::endl;
    }
}

void ItemFactory::LoadDropChances(const std::string mapUuid)
{
    auto it = dropChances_.find(mapUuid);
    if (it != dropChances_.end())
        // Already loaded
        return;

    IO::DataClient* client = GetSubsystem<IO::DataClient>();
    AB::Entities::ItemChanceList il;
    il.uuid = mapUuid;
    if (!client->Read(il))
    {
        LOG_ERROR << "Failed to read Item list for map " << mapUuid << std::endl;
        return;
    }

    std::unique_ptr<ItemSelector> selector = std::make_unique<ItemSelector>();
    for (const auto& i : il.items)
    {
        selector->Add(i.first, i.second);
    }
    selector->Update();

    std::lock_guard<std::mutex> lockClass(lock_);
    dropChances_.emplace(mapUuid, std::move(selector));
}

std::unique_ptr<Item> ItemFactory::CreateDropItem(const std::string& mapUuid, uint32_t level, const std::string& playerUuid)
{
    auto it = dropChances_.find(mapUuid);
    if (it != dropChances_.end())
        return std::unique_ptr<Item>();
    if ((*it).second->Count() == 0)
        return std::unique_ptr<Item>();

    auto rng = GetSubsystem<Crypto::Random>();
    const float rnd1 = rng->GetFloat();
    const float rnd2 = rng->GetFloat();
    std::string itemUuid = (*it).second->Get(rnd1, rnd2);
    if (uuids::uuid(itemUuid).nil())
        // There is a chance that nothing drops
        return std::unique_ptr<Item>();

    return CreateItem(itemUuid, level, Utils::Uuid::EMPTY_UUID, playerUuid);
}


}

#include "stdafx.h"
#include "ItemDrop.h"
#include "Item.h"
#include "Actor.h"
#include "ItemFactory.h"
#include "Subsystems.h"
#include "Game.h"

namespace Game {

void ItemDrop::RegisterLua(kaguya::State& state)
{
    state["ItemDrop"].setClass(kaguya::UserdataMetatable<ItemDrop>()
    );
}

ItemDrop::ItemDrop(std::unique_ptr<Item>& item) :
    GameObject(),
    item_(std::move(item))
{
    // Drops can not hide other objects
    occluder_ = false;
    name_ = item_->data_.name;
}

ItemDrop::~ItemDrop()
{
    if (!pickedUp_ && item_)
    {
        // Not picked up delete it
        auto* factory = GetSubsystem<ItemFactory>();
        factory->DeleteConcrete(item_->concreteItem_.uuid);
    }
}

uint32_t ItemDrop::GetItemIndex() const
{
    return item_ ? item_->data_.index : 0;
}

void ItemDrop::PickUp(Actor* actor)
{
    // if actorId_ == 0 all can pick it up
    if (actorId_ != 0 && actorId_ != actor->GetId())
        return;
    if (IsInRange(Ranges::Adjecent, actor))
    {
        if (actor->AddToInventory(item_))
        {
            pickedUp_ = true;
            actor->SetSelectedObjectById(0);
            Remove();
        }
    }
}

uint32_t ItemDrop::GetSourceId()
{
    if (auto s = source_.lock())
        return s->id_;
    return 0;
}

void ItemDrop::OnSelected(Actor* actor)
{
    if (!actor)
        return;
    if (!item_)
        return;
    PickUp(actor);
}

void ItemDrop::OnClicked(Actor* actor)
{
    if (!actor)
        return;
    if (!item_)
        return;
    if (auto o = actor->selectedObject_.lock())
    {
        if (o->id_ == id_)
            PickUp(actor);
    }
}

void ItemDrop::SetSource(std::shared_ptr<Actor> source)
{
    source_ = source;
}

bool ItemDrop::Serialize(IO::PropWriteStream& stream)
{
    if (!GameObject::Serialize(stream))
        return false;
    stream.Write<uint32_t>(0);                                   // Level
    stream.Write<uint8_t>(AB::Entities::CharacterSexUnknown);
    stream.Write<uint32_t>(0);                                   // Prof 1
    stream.Write<uint32_t>(0);                                   // Prof 2
    stream.Write<uint32_t>(item_->data_.index);                  // Model index
    return true;
}

void ItemDrop::WriteSpawnData(Net::NetworkMessage& msg)
{
    msg.Add<uint32_t>(id_);
    msg.Add<float>(transformation_.position_.x_);
    msg.Add<float>(transformation_.position_.y_);
    msg.Add<float>(transformation_.position_.z_);
    msg.Add<float>(transformation_.GetYRotation());
    msg.Add<float>(transformation_.scale_.x_);
    msg.Add<float>(transformation_.scale_.y_);
    msg.Add<float>(transformation_.scale_.z_);
    msg.Add<uint8_t>(1);                                  // not destroyable
    msg.Add<uint8_t>(stateComp_.GetState());
    msg.Add<float>(0.0f);                                 // speed
    msg.Add<uint32_t>(0);                                 // Group id
    msg.Add<uint8_t>(0);                                  // Group pos
    IO::PropWriteStream data;
    size_t dataSize;
    Serialize(data);
    const char* cData = data.GetStream(dataSize);
    msg.AddString(std::string(cData, dataSize));
}

}

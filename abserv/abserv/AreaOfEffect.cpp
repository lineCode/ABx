#include "stdafx.h"
#include "AreaOfEffect.h"
#include "Subsystems.h"
#include "DataProvider.h"
#include "ScriptManager.h"
#include "Actor.h"
#include "Game.h"

namespace Game {

void AreaOfEffect::InitializeLua()
{
    ScriptManager::RegisterLuaAll(luaState_);
    luaState_["self"] = this;
    luaInitialized_ = true;
}

void AreaOfEffect::_LuaSetSource(Actor* source)
{
    if (source)
        source_ = source->GetThis<Actor>();
    else
        source_.reset();
}

Actor* AreaOfEffect::_LuaGetSource()
{
    auto source = GetSource();
    if (source)
        return source.get();
    return nullptr;
}

void AreaOfEffect::RegisterLua(kaguya::State& state)
{
    state["AreaOfEffect"].setClass(kaguya::UserdataMetatable<AreaOfEffect, GameObject>()
        .addFunction("SetRange", &AreaOfEffect::SetRange)
        .addFunction("GetRange", &AreaOfEffect::GetRange)
        .addFunction("SetShapeType", &AreaOfEffect::SetShapeType)
        .addFunction("GetShapeType", &AreaOfEffect::GetShapeType)
        .addFunction("GetSource", &AreaOfEffect::_LuaGetSource)
        .addFunction("SetSource", &AreaOfEffect::_LuaSetSource)
        .addFunction("GetStartTime", &AreaOfEffect::GetStartTime)
        .addFunction("GetLifetime", &AreaOfEffect::GetLifetime)
        .addFunction("SetLifetime", &AreaOfEffect::SetLifetime)
        .addFunction("GetItemIndex", &AreaOfEffect::GetItemIndex)
        .addFunction("SetItemIndex", &AreaOfEffect::SetItemIndex)

        .addFunction("Index", &AreaOfEffect::GetIndex)
    );
}

AreaOfEffect::AreaOfEffect() :
    GameObject(),
    startTime_(Utils::Tick()),
    lifetime_(std::numeric_limits<uint32_t>::max())
{
    // By default AOE has a sphere shape with the range as radius
    SetCollisionShape(
        std::make_unique<Math::CollisionShapeImpl<Math::Sphere>>(Math::ShapeType::Sphere,
            Math::Vector3::Zero, RangeDistances[static_cast<int>(range_)] * 0.5f)
    );
    // AOE can not hide other objects
    occluder_ = false;
    // AOE usually not colliding
    collisionMask_ = 0;

    InitializeLua();
}

bool AreaOfEffect::LoadScript(const std::string& fileName)
{
    script_ = GetSubsystem<IO::DataProvider>()->GetAsset<Script>(fileName);
    if (!script_)
        return false;
    if (!script_->Execute(luaState_))
        return false;

    itemIndex_ = luaState_["itemIndex"];
    if (ScriptManager::IsNumber(luaState_, "creatureState"))
        stateComp_.SetState(luaState_["creatureState"], true);
    else
        stateComp_.SetState(AB::GameProtocol::CreatureStateIdle, true);

    if (ScriptManager::IsFunction(luaState_, "onUpdate"))
        functions_ |= FunctionUpdate;
    if (ScriptManager::IsFunction(luaState_, "onEnded"))
        functions_ |= FunctionEnded;
    if (ScriptManager::IsFunction(luaState_, "onCollide"))
        functions_ |= FunctionOnCollide;
    if (ScriptManager::IsFunction(luaState_, "onTrigger"))
        functions_ |= FunctionOnTrigger;
    if (ScriptManager::IsFunction(luaState_, "onLeftArea"))
        functions_ |= FunctionOnLeftArea;

    bool ret = luaState_["onInit"]();
    return ret;
}

void AreaOfEffect::Update(uint32_t timeElapsed, Net::NetworkMessage& message)
{
    GameObject::Update(timeElapsed, message);

    stateComp_.Update(timeElapsed);

    stateComp_.Write(message);

    if (HaveFunction(FunctionUpdate))
        ScriptManager::CallFunction(luaState_, "onUpdate", timeElapsed);
    if (Utils::TimeElapsed(startTime_) > lifetime_)
    {
        if (HaveFunction(FunctionEnded))
            ScriptManager::CallFunction(luaState_, "onEnded");
        Remove();
    }
}

void AreaOfEffect::OnCollide(GameObject* other)
{
    // Called from collisionComp_ of the moving object
    GameObject::OnCollide(other);

    // AOE can also be a trap for example
    if (HaveFunction(FunctionOnCollide))
        ScriptManager::CallFunction(luaState_, "onCollide", other);
}

void AreaOfEffect::OnTrigger(GameObject* other)
{
    // Called from our triggerComp_
    GameObject::OnTrigger(other);

    // AOE can also be a trap for example
    if (HaveFunction(FunctionOnTrigger))
        ScriptManager::CallFunction(luaState_, "onTrigger", other);
}

void AreaOfEffect::OnLeftArea(GameObject* other)
{
    // Called from triggerComp_
    GameObject::OnTrigger(other);
    // AOE can also be a trap for example
    if (HaveFunction(FunctionOnLeftArea))
        ScriptManager::CallFunction(luaState_, "onLeftArea", other);
}

Math::ShapeType AreaOfEffect::GetShapeType() const
{
    auto* shape = GetCollisionShape();
    if (!shape)
        return Math::ShapeType::None;
    return shape->shapeType_;
}

void AreaOfEffect::SetShapeType(Math::ShapeType shape)
{
    switch (shape)
    {
    case Math::ShapeType::BoundingBox:
    {
        const float rangeSize = RangeDistances[static_cast<int>(range_)] * 0.5f;
        const Math::Vector3 halfSize = { rangeSize, rangeSize, rangeSize };
        SetCollisionShape(
            std::make_unique<Math::CollisionShapeImpl<Math::BoundingBox>>(Math::ShapeType::BoundingBox,
                -halfSize, halfSize)
        );
        break;
    }
    case Math::ShapeType::Sphere:
        SetCollisionShape(
            std::make_unique<Math::CollisionShapeImpl<Math::Sphere>>(Math::ShapeType::Sphere,
                Math::Vector3::Zero, RangeDistances[static_cast<int>(range_)] * 0.5f)
        );
        break;
    case Math::ShapeType::None:
        SetCollisionShape(std::unique_ptr<Math::CollisionShape>());
        break;
    default:
        LOG_ERROR << "Invalid shape type for AOE " << static_cast<int>(shape) << std::endl;
        break;
    }
}

void AreaOfEffect::SetRange(Ranges range)
{
    // Size can also be set with GameObject::(_Lua)SetBoundingSize()
    if (range_ == range)
        return;

    range_ = range;

    // Update collision shape size
    auto cs = GetCollisionShape();
    if (!cs)
        return;

    switch (cs->shapeType_)
    {
    case Math::ShapeType::BoundingBox:
    {
        const float rangeSize = RangeDistances[static_cast<int>(range_)] * 0.5f;
        const Math::Vector3 halfSize = { rangeSize, rangeSize, rangeSize };
        using BoxShape = Math::CollisionShapeImpl<Math::BoundingBox>;
        BoxShape* shape = static_cast<BoxShape*>(cs);
        shape->Object()->min_ = -halfSize;
        shape->Object()->max_ = halfSize;
        break;
    }
    case Math::ShapeType::Sphere:
    {
        using SphereShape = Math::CollisionShapeImpl<Math::Sphere>;
        SphereShape* shape = static_cast<SphereShape*>(cs);
        shape->Object()->radius_ = RangeDistances[static_cast<int>(range)] * 0.5f;
        break;
    }
    default:
        LOG_WARNING << "Can not set range for shape type " << static_cast<int>(cs->shapeType_) << std::endl;
        break;
    }
}

void AreaOfEffect::SetSource(std::shared_ptr<Actor> source)
{
    source_ = source;
}

std::shared_ptr<Actor> AreaOfEffect::GetSource()
{
    return source_.lock();
}

uint32_t AreaOfEffect::GetGroupId() const
{
    if (auto s = source_.lock())
        return s->GetGroupId();
    return 0u;
}

uint32_t AreaOfEffect::GetItemIndex() const
{
    return itemIndex_;
}

bool AreaOfEffect::Serialize(IO::PropWriteStream& stream)
{
    if (!GameObject::Serialize(stream))
        return false;
    stream.Write<uint32_t>(0);                                   // Level
    stream.Write<uint8_t>(AB::Entities::CharacterSexUnknown);
    stream.Write<uint32_t>(0);                                   // Prof 1
    stream.Write<uint32_t>(0);                                   // Prof 2
    stream.Write<uint32_t>(GetItemIndex());   // Model/Item index
    return true;
}

void AreaOfEffect::WriteSpawnData(Net::NetworkMessage& msg)
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
    msg.Add<uint32_t>(GetGroupId());                                 // Group id
    msg.Add<uint8_t>(0);                                  // Group pos
    IO::PropWriteStream data;
    size_t dataSize;
    Serialize(data);
    const char* cData = data.GetStream(dataSize);
    msg.AddString(std::string(cData, dataSize));
}

}

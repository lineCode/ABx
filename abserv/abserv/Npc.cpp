#include "stdafx.h"
#include "Npc.h"
#include "GameManager.h"
#include "ScriptManager.h"
#include "MathUtils.h"
#include "DataProvider.h"
#include "Subsystems.h"
#include "AiCharacter.h"
#include "Random.h"
#include "Party.h"

namespace Game {

void Npc::InitializeLua()
{
    ScriptManager::RegisterLuaAll(luaState_);
    luaState_["self"] = this;
    luaInitialized_ = true;
}

void Npc::RegisterLua(kaguya::State& state)
{
    state["Npc"].setClass(kaguya::UserdataMetatable<Npc, Actor>()
        .addFunction("IsServerOnly", &Npc::IsServerOnly)
        .addFunction("SetServerOnly", &Npc::SetServerOnly)

        .addFunction("SetLevel", &Npc::SetLevel)             // Can only be used in onInit(), i.e. before it is sent to the clients
        .addFunction("SetBehaviour", &Npc::SetBehaviour)
        .addFunction("GetBehaviour", &Npc::GetBehaviour)
        .addFunction("Say", &Npc::Say)
        .addFunction("GetGroupId", &Npc::GetGroupId)
        .addFunction("SetGroupId", &Npc::SetGroupId)
    );
}

Npc::Npc() :
    Actor(),
    aiCharacter_(nullptr),
    luaInitialized_(false)
{
    // Party and Groups must be unique, i.e. share the same ID pool.
    groupId_ = Party::GetNewId();
    InitializeLua();
}

Npc::~Npc()
{
    Shutdown();
}

bool Npc::LoadScript(const std::string& fileName)
{
    script_ = GetSubsystem<IO::DataProvider>()->GetAsset<Script>(fileName);
    if (!script_)
        return false;
    if (!script_->Execute(luaState_))
        return false;

    name_ = (const char*)luaState_["name"];
    level_ = luaState_["level"];
    itemIndex_ = luaState_["itemIndex"];
    if (ScriptManager::IsNumber(luaState_, "sex"))
        sex_ = luaState_["sex"];
    if (ScriptManager::IsNumber(luaState_, "group_id"))
        groupId_ = luaState_["group_id"];

    if (ScriptManager::IsNumber(luaState_, "creatureState"))
        stateComp_.SetState(luaState_["creatureState"], true);
    else
        stateComp_.SetState(AB::GameProtocol::CreatureStateIdle, true);

    IO::DataClient* client = GetSubsystem<IO::DataClient>();

    if (ScriptManager::IsNumber(luaState_, "prof1Index"))
    {
        skills_->prof1_.index = luaState_["prof1Index"];
        if (skills_->prof1_.index != 0)
        {
            if (!client->Read(skills_->prof1_))
            {
                LOG_WARNING << "Unable to read primary profession of " << GetName() << ", index = " << skills_->prof1_.index << std::endl;
            }
        }
    }
    if (ScriptManager::IsNumber(luaState_, "prof2Index"))
    {
        skills_->prof2_.index = luaState_["prof2Index"];
        if (skills_->prof2_.index != 0)
        {
            if (!client->Read(skills_->prof2_))
            {
                LOG_WARNING << "Unable to read secondary profession of " << GetName() << ", index = " << skills_->prof2_.index << std::endl;
            }
        }
    }

    if (ScriptManager::IsString(luaState_, "behavior"))
        behaviorTree_ = (const char*)luaState_["behavior"];
    if (ScriptManager::IsFunction(luaState_, "onUpdate"))
        functions_ |= FunctionUpdate;
    if (ScriptManager::IsFunction(luaState_, "onTrigger"))
        functions_ |= FunctionOnTrigger;
    if (ScriptManager::IsFunction(luaState_, "onLeftArea"))
        functions_ |= FunctionOnLeftArea;

    // Initialize resources, etc. may be overwritten in onInit() in the NPC script bellow.
    Initialize();

    if (!behaviorTree_.empty())
        SetBehaviour(behaviorTree_);

    return luaState_["onInit"]();
}

std::shared_ptr<ai::AI> Npc::GetAi()
{
    if (!ai_)
        return std::shared_ptr<ai::AI>();

    if (!aiCharacter_)
    {
        aiCharacter_ = std::make_shared<AI::AiCharacter>(*this);
        ai_->setCharacter(aiCharacter_);
    }
    return ai_;
}

void Npc::Shutdown()
{
    if (ai_)
    {
        ai::Zone* zone = ai_->getZone();
        if (zone == nullptr)
            return;
        zone->destroyAI(static_cast<int>(id_));
        ai_->setZone(nullptr);
    }
}

void Npc::SetLevel(uint32_t value)
{
    level_ = value;
    resourceComp_.UpdateResources();
}

void Npc::Update(uint32_t timeElapsed, Net::NetworkMessage& message)
{
    Actor::Update(timeElapsed, message);
    if (luaInitialized_ && HaveFunction(FunctionUpdate))
        luaState_["onUpdate"](timeElapsed);
    if (aiCharacter_)
    {
        const Math::Vector3& pos = transformation_.position_;
        aiCharacter_->setPosition({ pos.x_, pos.y_, pos.z_ });
    }
    ScriptManager::CollectGarbage(luaState_);
}

void Npc::SetGroupId(uint32_t value)
{
    if (groupId_ != value)
    {
        auto ai = GetAi();
        if (ai)
            GetGame()->map_->SetEntityGroupId(ai, groupId_, value);
        groupId_ = value;
    }
}

bool Npc::SetBehaviour(const std::string& name)
{
    if (behaviorTree_.compare(name) != 0 || !ai_)
    {
        behaviorTree_ = name;
        auto* loader = GetSubsystem<AI::AiLoader>();
        const ai::TreeNodePtr& root = loader->Load(behaviorTree_);
        if (!root)
            return false;
        if (ai_)
            ai_->setBehaviour(root);
        else
        {
            ai_ = std::make_shared<ai::AI>(root);
            ai_->getAggroMgr().setReduceByValue(0.1f);
            GetGame()->map_->AddEntity(GetAi(), groupId_);
        }
    }
    return true;
}

float Npc::GetAggro(const Actor* other)
{
    if (!other)
        return 0.0f;

    auto* random = GetSubsystem<Crypto::Random>();
    const float dist = GetPosition().Distance(other->GetPosition());
    const float rval = random->GetFloat();
    return (1.0f / dist) * rval;
}

void Npc::WriteSpawnData(Net::NetworkMessage& msg)
{
    if (!serverOnly_)
        Actor::WriteSpawnData(msg);
}

void Npc::Say(ChatType channel, const std::string& message)
{
    switch (channel)
    {
    case ChatType::Map:
    {
        std::shared_ptr<ChatChannel> ch = GetSubsystem<Chat>()->Get(ChatType::Map, static_cast<uint64_t>(GetGame()->id_));
        if (ch)
            ch->TalkNpc(this, message);
        break;
    }
    case ChatType::Party:
    {
        std::shared_ptr<ChatChannel> ch = GetSubsystem<Chat>()->Get(ChatType::Party, GetGroupId());
        if (ch)
            ch->TalkNpc(this, message);
        break;
    }
    default:
        // N/A
        break;
    }
}

void Npc::OnSelected(Actor* selector)
{
    Actor::OnSelected(selector);
    if (luaInitialized_ && selector)
        ScriptManager::CallFunction(luaState_, "onSelected", selector);
}

void Npc::OnClicked(Actor* selector)
{
    Actor::OnSelected(selector);
    if (luaInitialized_ && selector)
        ScriptManager::CallFunction(luaState_, "onClicked", selector);
}

void Npc::OnArrived()
{
    Actor::OnArrived();
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onArrived");
}

void Npc::OnCollide(GameObject* other)
{
    Actor::OnCollide(other);

    if (luaInitialized_ && other)
        ScriptManager::CallFunction(luaState_, "onCollide", other);
}

void Npc::OnTrigger(GameObject* other)
{
    // Called from triggerComp_
    Actor::OnTrigger(other);

    if (luaInitialized_ && HaveFunction(FunctionOnTrigger))
        ScriptManager::CallFunction(luaState_, "onTrigger", other);
}

void Npc::OnLeftArea(GameObject* other)
{
    // Called from triggerComp_
    Actor::OnLeftArea(other);

    if (luaInitialized_ && HaveFunction(FunctionOnLeftArea))
        ScriptManager::CallFunction(luaState_, "onLeftArea", other);
}

void Npc::OnEndUseSkill(Skill* skill)
{
    Actor::OnEndUseSkill(skill);

    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onEndUseSkill", skill);
}

void Npc::OnStartUseSkill(Skill* skill)
{
    Actor::OnStartUseSkill(skill);

    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onStartUseSkill", skill);
}

bool Npc::OnAttack(Actor* target)
{
    bool ret = Actor::OnAttack(target);

    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onAttack", target, ret);
    return ret;
}

bool Npc::OnAttacked(Actor* source, DamageType type, int32_t damage)
{
    bool ret = Actor::OnAttacked(source, type, damage);

    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onAttacked", source, type, damage, ret);
    if (ret)
    {
        if (aiCharacter_)
        {
            aiCharacter_->lastFoeAttack_ = Utils::Tick();
            aiCharacter_->lastAttacker_ = source->GetThis<Actor>();
        }
    }
    return ret;
}

bool Npc::OnGettingAttacked(Actor* source)
{
    bool ret = Actor::OnGettingAttacked(source);

    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onGettingAttacked", source, ret);
    return ret;
}

bool Npc::OnUseSkill(Actor* target, Skill* skill)
{
    bool ret = Actor::OnUseSkill(target, skill);

    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onUseSkill", target, skill, ret);
    return ret;
}

bool Npc::OnSkillTargeted(Actor* source, Skill* skill)
{
    bool ret = Actor::OnSkillTargeted(source, skill);

    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onSkillTargeted", source, skill, ret);
    return ret;
}

bool Npc::OnInterruptingAttack()
{
    bool ret = Actor::OnInterruptingAttack();
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onInterruptingAttack", ret);
    return ret;
}

bool Npc::OnInterruptingSkill(AB::Entities::SkillType type, Skill* skill)
{
    bool ret = Actor::OnInterruptingSkill(type, skill);
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onInterruptingSkill", type, skill, ret);
    return ret;
}

void Npc::OnInterruptedAttack()
{
    Actor::OnInterruptedAttack();
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onInterruptedAttack");
}

void Npc::OnInterruptedSkill(Skill* skill)
{
    Actor::OnInterruptedSkill(skill);
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onInterruptedSkill", skill);
}

void Npc::OnKnockedDown(uint32_t time)
{
    Actor::OnKnockedDown(time);
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onKnockedDown", time);
}

void Npc::OnHealed(int hp)
{
    Actor::OnHealed(hp);
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onHealed", hp);
}

void Npc::OnDied()
{
    Actor::OnDied();
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onDied");
}

void Npc::OnResurrected(int health, int energy)
{
    Actor::OnResurrected(health, energy);
    if (luaInitialized_)
        ScriptManager::CallFunction(luaState_, "onResurrected");
}

}

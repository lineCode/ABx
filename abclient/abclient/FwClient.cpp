#include "stdafx.h"
#include "FwClient.h"
#include "AbEvents.h"
#include "LevelManager.h"
#include "BaseLevel.h"
#include <AB/ProtocolCodes.h>
#include "Options.h"

String FwClient::GetProtocolErrorMessage(uint8_t err)
{
    switch (err)
    {
    case AB::Errors::IPBanned:
        return "Your IP Address is banned";
    case AB::Errors::TooManyConnectionsFromThisIP:
        return "Too many connection from this IP";
    case AB::Errors::InvalidAccountName:
        return "Invalid Account name";
    case AB::Errors::InvalidPassword:
        return "Invalid password";
    case AB::Errors::NamePasswordMismatch:
        return "Name or password wrong";
    case AB::Errors::AlreadyLoggedIn:
        return "You are already logged in";
    case AB::Errors::ErrorLoadingCharacter:
        return "Error loading character";
    case AB::Errors::AccountBanned:
        return "Your account is banned";
    case AB::Errors::WrongProtocolVersion:
        return "Outdated client. Please update the game client.";
    default:
        return "";
    }
}

FwClient::FwClient(Context* context) :
    Object(context),
    loggedIn_(false),
    playerId_(0)
{
    Options* o = context->GetSubsystem<Options>();
    client_.loginHost_ = std::string(o->loginHost_.CString());
    client_.loginPort_ = o->loginPort_;
    client_.receiver_ = this;
    lastState_ = client_.state_;
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(FwClient, HandleUpdate));
    SubscribeToEvent(AbEvents::E_LEVEL_READY, URHO3D_HANDLER(FwClient, HandleLevelReady));
}

FwClient::~FwClient()
{
    client_.receiver_ = nullptr;
    UnsubscribeFromAllEvents();
}

bool FwClient::Start()
{
    return true;
}

void FwClient::Stop()
{
    Logout();
}

void FwClient::HandleLevelReady(StringHash eventType, VariantMap& eventData)
{
    String levelName = eventData[AbEvents::E_LEVEL_READY].GetString();
    levelReady_ = true;
    // Level loaded, send queued events
    for (auto& e : queuedEvents_)
    {
        SendEvent(e.eventId, e.eventData);
    }
    queuedEvents_.Clear();
}

void FwClient::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    client_.Update(static_cast<int>(timeStep * 1000));
    if (lastState_ == client_.state_)
        return;

    switch (client_.state_)
    {
    case Client::Client::StateSelecChar:
        loggedIn_ = true;
        break;
    case Client::Client::StateWorld:
        break;
    }
    lastState_ = client_.state_;
}

void FwClient::Login(const String& name, const String& pass)
{
    if (!loggedIn_)
    {
        accountName_ = name;
        accounbtPass_ = pass;
        client_.Login(std::string(name.CString()), std::string(pass.CString()));
    }
}

void FwClient::EnterWorld(const String& charName, const String& map)
{
    if (loggedIn_)
    {
        client_.EnterWorld(std::string(charName.CString()), std::string(map.CString()));
    }
}

void FwClient::Logout()
{
    client_.Logout();
    loggedIn_ = false;
}

void FwClient::Move(uint8_t direction)
{
    if (loggedIn_)
        client_.Move(direction);
}

void FwClient::Turn(uint8_t direction)
{
    if (loggedIn_)
        client_.Turn(direction);
}

void FwClient::OnGetCharlist(const Client::CharList& chars)
{
    levelReady_ = false;
    characters_ = chars;
    VariantMap& eData = GetEventDataMap();
    currentLevel_ = "CharSelectLevel";
    eData[AbEvents::E_SET_LEVEL] = currentLevel_;
    SendEvent(AbEvents::E_SET_LEVEL, eData);
}

void FwClient::OnEnterWorld(const std::string& mapName, uint32_t playerId)
{
    levelReady_ = false;
    playerId_ = playerId;
    VariantMap& eData = GetEventDataMap();
    currentLevel_ = "OutpostLevel";
    eData[AbEvents::ED_MAP_NAME] = String(mapName.c_str());
    eData[AbEvents::E_SET_LEVEL] = currentLevel_;
    SendEvent(AbEvents::E_SET_LEVEL, eData);

    Graphics* graphics = GetSubsystem<Graphics>();
    graphics->SetWindowTitle("FW - " + accountName_);
}

void FwClient::OnNetworkError(const std::error_code& err)
{
    loggedIn_ = false;
    LevelManager* lm = context_->GetSubsystem<LevelManager>();
    BaseLevel* cl = static_cast<BaseLevel*>(lm->GetCurrentLevel());

    cl->OnNetworkError(err);

    if (lm->GetLevelName() != "LoginLevel")
    {
        // Disconnect -> Relogin
        VariantMap& eData = GetEventDataMap();
        eData[AbEvents::E_SET_LEVEL] = "LoginLevel";
        SendEvent(AbEvents::E_SET_LEVEL, eData);
    }
}

void FwClient::QueueEvent(StringHash eventType, VariantMap& eventData)
{
    if (levelReady_)
        SendEvent(eventType, eventData);
    else
        queuedEvents_.Push({ eventType, eventData });
}

void FwClient::OnProtocolError(uint8_t err)
{
    LevelManager* lm = context_->GetSubsystem<LevelManager>();
    BaseLevel* cl = static_cast<BaseLevel*>(lm->GetCurrentLevel());
    cl->OnProtocolError(err);
}

void FwClient::OnSpawnObject(uint32_t id, const Vec3& pos, const Vec3& scale, float rot,
    PropReadStream& data, bool existing)
{
    VariantMap& eData = GetEventDataMap();
    eData[AbEvents::ED_OBJECT_ID] = id;
    eData[AbEvents::ED_POS] = Vector3(pos.x, pos.y, pos.z);
    eData[AbEvents::ED_ROTATION] = rot;
    eData[AbEvents::ED_SCALE] = Vector3(scale.x, scale.y, scale.z);
    String d(data.Buffer(), static_cast<unsigned>(data.GetSize()));
    eData[AbEvents::ED_OBJECT_DATA] = d;
    if (!existing)
        QueueEvent(AbEvents::E_OBJECT_SPAWN, eData);
    else
        QueueEvent(AbEvents::E_OBJECT_SPAWN_EXISTING, eData);
}

void FwClient::OnDespawnObject(uint32_t id)
{
    VariantMap& eData = GetEventDataMap();
    eData[AbEvents::ED_OBJECT_ID] = id;
    QueueEvent(AbEvents::E_OBJECT_DESPAWN, eData);
}

void FwClient::OnObjectPos(uint32_t id, const Vec3& pos)
{
    VariantMap& eData = GetEventDataMap();
    eData[AbEvents::ED_OBJECT_ID] = id;
    eData[AbEvents::ED_POS] = Vector3(pos.x, pos.y, pos.z);
    QueueEvent(AbEvents::E_OBJECT_POS_UPDATE, eData);
}

void FwClient::OnObjectRot(uint32_t id, float rot)
{
    VariantMap& eData = GetEventDataMap();
    eData[AbEvents::ED_OBJECT_ID] = id;
    eData[AbEvents::ED_ROTATION] = rot;
    QueueEvent(AbEvents::E_OBJECT_ROT_UPDATE, eData);
}
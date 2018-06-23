#pragma once

class Actor;

enum class PartyWindowMode
{
    ModeOutpost,
    ModeGame
};

class PartyWindow : public Window
{
    URHO3D_OBJECT(PartyWindow, Window);
private:
    PartyWindowMode mode_;
    SharedPtr<Window> window_;
    SharedPtr<LineEdit> addPlayerEdit_;
    int memberCount_;
    SharedPtr<UIElement> memberContainer_;
    SharedPtr<UIElement> partyContainer_;
    void HandleAddTargetClicked(StringHash eventType, VariantMap& eventData);
    void HandleCloseClicked(StringHash eventType, VariantMap& eventData);
    void HandleObjectSelected(StringHash eventType, VariantMap& eventData);
    void HandlePartyInvited(StringHash eventType, VariantMap& eventData);
    void HandlePartyAdded(StringHash eventType, VariantMap& eventData);
    void HandlePartyInviteRemoved(StringHash eventType, VariantMap& eventData);
    void HandlePartyRemoved(StringHash eventType, VariantMap& eventData);
    void SubscribeEvents();
public:
    static void RegisterObject(Context* context);

    PartyWindow(Context* context);
    ~PartyWindow();

    void SetMode(PartyWindowMode mode);
    void AddActor(SharedPtr<Actor> actor);
};


// Level manager, fading [artgolf1000](https://urho3d.prophpbb.com/topic2367.html)

#pragma once

#include "Player.h"
#include "PostProcessController.h"
#include "Errors.h"
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Navigation/NavigationMesh.h>

class FwClient;

class BaseLevel : public Object
{
    URHO3D_OBJECT(BaseLevel, Object);
public:
    BaseLevel(Context* context) :
        Object(context),
        uiRoot_(GetSubsystem<UI>()->GetRoot()),
        scene_(nullptr),
        cameraNode_(nullptr),
        player_(nullptr),
        yaw_(0.0f),
        pitch_(0.0f),
        debugGeometry_(false)
    {}

    virtual ~BaseLevel()
    {
        UnsubscribeFromAllEvents();
        Dispose();
    }

    virtual void Run();
    virtual void Pause();
protected:
    friend class FwClient;
    virtual void CreateUI();
    virtual void CreateScene();
    virtual void Dispose();
    virtual void SubscribeToEvents();
    virtual void SetupViewport();
    void CreateLogo();
    virtual void Update(StringHash, VariantMap&) {}
    virtual void PostUpdate(StringHash, VariantMap&) {}
    virtual void PostRenderUpdate(StringHash eventType, VariantMap& eventData);

    virtual void OnNetworkError(Client::ConnectionError connectionError, const std::error_code& err);
    virtual void OnProtocolError(uint8_t err);

    Ray GetActiveViewportScreenRay(const IntVector2& pos) const
    {
        if (viewport_)
            return viewport_->GetScreenRay(pos.x_, pos.y_);
        return Ray();
    }
    void InitSunProperties();
    void InitModelAnimations();

    Urho3D::UIElement* uiRoot_;
    SharedPtr<Scene> scene_;
    SharedPtr<Node> cameraNode_;
    SharedPtr<Player> player_;
    SharedPtr<Viewport> viewport_;
    SharedPtr<PostProcessController> postProcess_;
    /// Camera yaw angle.
    float yaw_;
    /// Camera pitch angle.
    float pitch_;
    SharedPtr<Sprite> logoSprite_;
public:
    bool debugGeometry_;
    /// If this level has a player call this to create it
    void ToggleDebugGeometry()
    {
        debugGeometry_ = !debugGeometry_;
    }
    virtual void ShowError(const String& message, const String& title = "Error");
    SharedPtr<Player> GetPlayer() const { return player_; }
    Camera* GetCamera() const
    {
        if (cameraNode_)
            return cameraNode_->GetComponent<Camera>();
        return nullptr;
    }
    PostProcessController* GetPostProcessController() const
    {
        if (postProcess_)
            return postProcess_.Get();
        return nullptr;
    }
private:
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
};

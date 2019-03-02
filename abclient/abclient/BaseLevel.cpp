#include "stdafx.h"
#include "BaseLevel.h"
#include "AbEvents.h"
#include "LevelManager.h"
#include <Urho3D/UI/MessageBox.h>
#include "FwClient.h"
#include "ClientApp.h"
#include "Ocean.h"

#include <Urho3D/DebugNew.h>

void BaseLevel::Run()
{
    if (scene_)
    {
        scene_->SetUpdateEnabled(true);
    }
}

void BaseLevel::Pause()
{
    if (scene_)
    {
        scene_->SetUpdateEnabled(false);
    }
}

void BaseLevel::Dispose()
{
    // Pause the scene, remove all contents from the scene, then remove the scene itself.
    if (scene_)
    {
        scene_->SetUpdateEnabled(false);
        scene_->Clear();
        scene_->Remove();
        scene_ = nullptr;
    }
}

void BaseLevel::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(BaseLevel, HandleUpdate));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(BaseLevel, HandlePostUpdate));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(BaseLevel, HandlePostRenderUpdate));
}

void BaseLevel::ShowError(const String& message, const String& title)
{
    using MsgBox = Urho3D::MessageBox;
    /* MsgBox* msgBox = */ new MsgBox(context_, message, title);
}

void BaseLevel::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if (oceanNode_)
    {
        BoundingBox bbox = ocean_->GetBoundingBox();

        if ((bbox.Size() - boundingBox_.Size()).Length() > 10.0f)
        {
            boundingBox_.Merge(bbox);
            staticModelOcean_->DSetBoundingBox(boundingBox_);
        }
    }

    Update(eventType, eventData);
}

void BaseLevel::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    PostUpdate(eventType, eventData);
}

void BaseLevel::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    PostRenderUpdate(eventType, eventData);
}

void BaseLevel::PostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    UNREFERENCED_PARAMETER(eventType);
    UNREFERENCED_PARAMETER(eventData);

    if (debugGeometry_)
    {
        if (auto physW = scene_->GetComponent<PhysicsWorld>())
            physW->DrawDebugGeometry(true);
        if (auto navMesh = scene_->GetComponent<NavigationMesh>())
            navMesh->DrawDebugGeometry(true);
    }
}

void BaseLevel::OnNetworkError(const std::error_code& err)
{
    URHO3D_LOGERRORF("Network error (%d): %s", err.value(), err.message().c_str());
    ShowError(String(err.message().c_str()), "Network Error");
}

void BaseLevel::OnProtocolError(uint8_t err)
{
    String msg = FwClient::GetProtocolErrorMessage(err);
    URHO3D_LOGERRORF("Protocol error (%d): %s", err, msg.CString());
    if (!msg.Empty())
        ShowError(msg, "Error");
}

void BaseLevel::SetSunProperties()
{
    // https://discourse.urho3d.io/t/better-shadows-possible-three-issues/1013/3
    // https://discourse.urho3d.io/t/shadow-on-slopes/4629
    Node* sunNode = scene_->GetChild("Sun", false);
    if (sunNode)
    {
        Light* sun = sunNode->GetComponent<Light>();
        if (sun)
        {
            sun->SetBrightness(1.0f);
            sun->SetShadowFadeDistance(100.0f);
            sun->SetShadowDistance(125.0f);

            sun->SetShadowBias(BiasParameters(0.0000025f, 1.0f));
            sun->SetShadowCascade(CascadeParameters(20.0f, 60.0f, 180.0f, 560.0f, 0.1f, 0.1f));
            //sun->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
            sun->SetShadowResolution(1.0);
            sun->SetCastShadows(true);
        }
    }
    else
        URHO3D_LOGWARNING("No Sun node found");
}

void BaseLevel::InitOcean()
{
#ifdef OCEAN_SIMULATION
    oceanNode_ = scene_->GetChild("Ocean", false);
    if (oceanNode_)
    {
#ifdef ADD_WATER_REFLECTION
        waterNode_ = scene_->CreateChild("Water");
        waterNode_->SetScale(Vector3(2048.0f, 1.0f, 2048.0f));
        waterNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
#endif
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        // Remove the static model
        oceanNode_->RemoveComponent<StaticModel>();
        oceanNode_->SetScale(Vector3(1.0f, 0.13f, 1.0f));
        ocean_ = oceanNode_->CreateComponent<Ocean>();
        ocean_->InitOcean();
        staticModelOcean_ = oceanNode_->CreateComponent<DStaticModel>();
        staticModelOcean_->SetModel(ocean_->GetOceanModel());
        staticModelOcean_->SetMaterial(cache->GetResource<Material>("Materials/Ocean.xml"));
        staticModelOcean_->SetViewMask(0x80000000);
    }
#endif
}

void BaseLevel::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();

    viewport_ = new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>());
    renderer->SetViewport(0, viewport_);

#if defined(OCEAN_SIMULATION) && defined(ADD_WATER_REFLECTION)
    // Ocean Water reflection
    Graphics* graphics = GetSubsystem<Graphics>();
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    if (oceanNode_)
    {
        // Create a mathematical plane to represent the water in calculations
        waterPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition());
        // Create a downward biased plane for reflection view clipping. Biasing is necessary to avoid too aggressive clipping
        waterClipPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition() -
            Vector3(0.0f, 0.1f, 0.0f));

        // Create camera for water reflection
        // It will have the same farclip and position as the main viewport camera, but uses a reflection plane to modify
        // its position when rendering
        reflectionCameraNode_ = cameraNode_->CreateChild();
        Camera* reflectionCamera = reflectionCameraNode_->CreateComponent<Camera>();
        reflectionCamera->SetFarClip(750.0);
        reflectionCamera->SetViewMask(0x7fffffff); // Hide objects with only bit 31 in the viewmask (the water plane)
        reflectionCamera->SetAutoAspectRatio(false);
        reflectionCamera->SetUseReflection(true);
        reflectionCamera->SetReflectionPlane(waterPlane_);
        reflectionCamera->SetUseClipping(true); // Enable clipping of geometry behind water plane
        reflectionCamera->SetClipPlane(waterClipPlane_);
        // The water reflection texture is rectangular. Set reflection camera aspect ratio to match
        reflectionCamera->SetAspectRatio((float)graphics->GetWidth() / (float)graphics->GetHeight());
        // View override flags could be used to optimize reflection rendering. For example disable shadows
        //reflectionCamera->SetViewOverrideFlags(VO_DISABLE_SHADOWS);

        // Create a texture and setup viewport for water reflection. Assign the reflection texture to the diffuse
        // texture unit of the water material
        int texSize = 1024;
        SharedPtr<Texture2D> renderTexture(new Texture2D(context_));
        renderTexture->SetSize(texSize, texSize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
        renderTexture->SetFilterMode(FILTER_BILINEAR);
        RenderSurface* surface = renderTexture->GetRenderSurface();
        SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, reflectionCamera));
        surface->SetViewport(0, rttViewport);
        Material* waterMat = cache->GetResource<Material>("Materials/Water.xml");
        waterMat->SetTexture(TU_DIFFUSE, renderTexture);
    }
    // /Ocean Water reflection
#endif

    postProcess_ = scene_->CreateComponent<PostProcessController>();
    postProcess_->AddViewport(viewport_, true);
    Options* options = GetSubsystem<Options>();
    postProcess_->SetUseFXAA3(options->GetAntiAliasingMode() == AntiAliasingMode::FXAA3);
}

void BaseLevel::CreateLogo()
{
    // Get logo texture
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Texture2D* logoTexture = cache->GetResource<Texture2D>("Textures/Trill.png");
    if (!logoTexture)
        return;

    // Create logo sprite and add to the UI layout
    UI* ui = GetSubsystem<UI>();
    logoSprite_ = ui->GetRoot()->CreateChild<Sprite>();

    // Set logo sprite texture
    logoSprite_->SetTexture(logoTexture);

    int textureWidth = logoTexture->GetWidth();
    int textureHeight = logoTexture->GetHeight();

    // Set logo sprite scale
    logoSprite_->SetScale(100.0f / textureWidth);

    // Set logo sprite size
    logoSprite_->SetSize(textureWidth, textureHeight);

    // Set logo sprite hot spot
    logoSprite_->SetHotSpot(textureWidth, textureHeight);

    // Set logo sprite alignment
    logoSprite_->SetAlignment(HA_RIGHT, VA_BOTTOM);

    // Make logo not fully opaque to show the scene underneath
    logoSprite_->SetOpacity(0.7f);

    // Set a low priority for the logo so that other UI elements can be drawn on top
    logoSprite_->SetPriority(-100);
}

void BaseLevel::CreateUI()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    XMLFile* style = cache->GetResource<XMLFile>("UI/FwDefaultStyle.xml");
    uiRoot_->SetDefaultStyle(style);
}

void BaseLevel::CreateScene()
{
    scene_ = new Scene(context_);
}

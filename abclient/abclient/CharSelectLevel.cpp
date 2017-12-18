#include "stdafx.h"
#include "CharSelectLevel.h"
#include "FwClient.h"
#include "Account.h"

CharSelectLevel::CharSelectLevel(Context* context) :
    BaseLevel(context)
{
    // Create the scene content
    CreateScene();
    CreateCamera();

    // Create the UI content
    CreateUI();
    CreateLogo();

    // Subscribe to global events for camera movement
    SubscribeToEvents();
}

void CharSelectLevel::CreateCamera()
{
    cameraNode_ = scene_->GetChild("CameraNode");
    if (!cameraNode_)
    {
        cameraNode_ = scene_->CreateChild("CameraNode");
        Camera* camera = cameraNode_->CreateComponent<Camera>();
        camera->SetFarClip(300.0f);
    }
    SetupViewport();
}

void CharSelectLevel::SubscribeToEvents()
{
    BaseLevel::SubscribeToEvents();
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(CharSelectLevel, HandleUpdate));
}

void CharSelectLevel::CreateUI()
{
    uiRoot_->RemoveAllChildren();
    BaseLevel::CreateUI();

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    Window* window = new Window(context_);
    uiRoot_->AddChild(window);
    window->SetSize(400, 300);
    window->SetPosition(0, -40);
    window->SetLayoutMode(LM_VERTICAL);
    window->SetLayoutSpacing(10);
    window->SetLayoutBorder(IntRect(10, 10, 10, 10));
    window->SetAlignment(HA_CENTER, VA_CENTER);
    window->SetName("Select Character");
    window->SetStyleAuto();

    FwClient* client = context_->GetSubsystem<FwClient>();
    const Client::CharList& chars = client->GetCharacters();
    int i = 0;
    for (const auto& ch : chars)
    {
        Button* button = new Button(context_);
        button->SetMinHeight(40);
        button->SetName(String(i));    // not required
        button->SetStyleAuto();
        button->SetOpacity(1.0f);     // transparency
        button->SetLayoutMode(LM_FREE);
        SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(CharSelectLevel, HandleCharClicked));
        {
            // buttons don't have a text by itself, a text needs to be added as a child
            Text* t = new Text(context_);
            t->SetAlignment(HA_CENTER, VA_CENTER);
            t->SetName("CharacterName");
            t->SetText(String(ch.name.c_str()));
            t->SetStyle("Text");
            button->AddChild(t);
        }
        window->AddChild(button);
        i++;
    }
}

void CharSelectLevel::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    scene_ = new Scene(context_);
    XMLFile *sceneFile = cache->GetResource<XMLFile>("Scenes/CharSelect.xml");
    scene_->LoadXML(sceneFile->GetRoot());
}

void CharSelectLevel::HandleCharClicked(StringHash eventType, VariantMap& eventData)
{
    Button* sender = static_cast<Button*>(eventData[Urho3D::Released::P_ELEMENT].GetVoidPtr());
    int index = std::atoi(sender->GetName().CString());
    FwClient* net = context_->GetSubsystem<FwClient>();
    String name = String(net->GetCharacters()[index].name.c_str());
    String map = String(net->GetCharacters()[index].lastMap.c_str());
    net->EnterWorld(name, map);
}

void CharSelectLevel::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    UNREFERENCED_PARAMETER(eventType);

    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();
    Quaternion rot;
    rot.FromAngleAxis(timeStep, Vector3(0.0f, 1.0f, 0.0f));
    cameraNode_->Rotate(rot);
}
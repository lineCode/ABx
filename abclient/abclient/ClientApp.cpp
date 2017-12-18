#include "stdafx.h"

#include "ClientApp.h"
#include "LevelManager.h"
#include "AbEvents.h"
#include "LoginLevel.h"
#include "OutpostLevel.h"
#include "PvpCombatLevel.h"
#include "CharSelectLevel.h"
#include "CharCreateLevel.h"
#include "FwClient.h"
#include "Player.h"
#include "Options.h"
#include "ChatWindow.h"

#include <Urho3D/DebugNew.h>

/**
* This macro is expanded to (roughly, depending on OS) this:
*
* > int RunApplication()
* > {
* > Urho3D::SharedPtr<Urho3D::Context> context(new Urho3D::Context());
* > Urho3D::SharedPtr<className> application(new className(context));
* > return application->Run();
* > }
* >
* > int main(int argc, char** argv)
* > {
* > Urho3D::ParseArguments(argc, argv);
* > return function;
* > }
*/
URHO3D_DEFINE_APPLICATION_MAIN(ClientApp)

/**
* This happens before the engine has been initialized
* so it's usually minimal code setting defaults for
* whatever instance variables you have.
* You can also do this in the Setup method.
*/
ClientApp::ClientApp(Context* context) :
    Application(context),
    framecount_(0),
    time_(0)
{
    // Register levels
    context->RegisterFactory<LoginLevel>();
    context->RegisterFactory<CharSelectLevel>();
    context->RegisterFactory<CharCreateLevel>();
    context->RegisterFactory<OutpostLevel>();
    context->RegisterFactory<PvpCombatLevel>();

    Options* options = new Options(context);
    options->Load();
    context->RegisterSubsystem(options);

    FwClient* cli = new FwClient(context);
    context->RegisterSubsystem(cli);
    LevelManager* lvl = new LevelManager(context);
    context->RegisterSubsystem(lvl);

    // Register factory and attributes for the Character component so it can
    // be created via CreateComponent, and loaded / saved
    Actor::RegisterObject(context);
    Player::RegisterObject(context);
    ChatWindow::RegisterObject(context);
}

/**
* This method is called before the engine has been initialized.
* Thusly, we can setup the engine parameters before anything else
* of engine importance happens (such as windows, search paths,
* resolution and other things that might be user configurable).
*/
void ClientApp::Setup()
{
    Options* options = GetSubsystem<Options>();
    // These parameters should be self-explanatory.
    // See http://urho3d.github.io/documentation/1.5/_main_loop.html
    // for a more complete list.
    //  Default 0 (use desktop resolution, or 1024 in windowed mode.)
    engineParameters_["WindowWidth"] = options->GetWidth();
    // Default 0 (use desktop resolution, or 768 in windowed mode.)
    engineParameters_["WindowHeight"] = options->GetHeight();
    engineParameters_["FullScreen"] = options->GetFullscreen();
    engineParameters_["Borderless"] = options->GetBorderless();
    engineParameters_["WindowResizable"] = options->GetResizeable();
    engineParameters_["HighDPI"] = options->GetHighDPI();
    engineParameters_["VSync"] = options->GetVSync();
    engineParameters_["TripleBuffer"] = options->GetTripleBuffer();
    engineParameters_["Multisample"] = options->GetMultiSample();
    engineParameters_["ResourcePaths"] = "AutoLoad;CoreData;Data;AbData";
    engineParameters_["LogName"] = "abclient.log";
}

/**
* This method is called after the engine has been initialized.
* This is where you set up your actual content, such as scenes,
* models, controls and what not. Basically, anything that needs
* the engine initialized and ready goes in here.
*/
void ClientApp::Start()
{
    SetRandomSeed(Time::GetSystemTime());
    SetWindowTitleAndIcon();

    GetSubsystem<Input>()->SetMouseVisible(true);
    GetSubsystem<Input>()->SetMouseGrabbed(false);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    // Let's use the default style that comes with Urho3D.
    GetSubsystem<UI>()->GetRoot()->SetDefaultStyle(cache->GetResource<XMLFile>("UI/FwDefaultStyle.xml"));

    // We subscribe to the events we'd like to handle.
    // In this example we will be showing what most of them do,
    // but in reality you would only subscribe to the events
    // you really need to handle.
    // These are sort of subscribed in the order in which the engine
    // would send the events. Read each handler method's comment for
    // details.
    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(ClientApp, HandleBeginFrame));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(ClientApp, HandleKeyDown));
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(ClientApp, HandleKeyDown));
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ClientApp, HandleUpdate));
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(ClientApp, HandlePostUpdate));
    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(ClientApp, HandleRenderUpdate));
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(ClientApp, HandlePostRenderUpdate));
    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(ClientApp, HandleEndFrame));

    SwitchScene("LoginLevel");
}

/**
* Good place to get rid of any system resources that requires the
* engine still initialized. You could do the rest in the destructor,
* but there's no need, this method will get called when the engine stops,
* for whatever reason (short of a segfault).
*/
void ClientApp::Stop()
{
    FwClient* cli = context_->GetSubsystem<FwClient>();
    cli->Logout();
}

/**
* Every frame's life must begin somewhere. Here it is.
*/
void ClientApp::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    // We really don't have anything useful to do here for this example.
    // Probably shouldn't be subscribing to events we don't care about.
}

/**
* Input from keyboard is handled here. I'm assuming that Input, if
* available, will be handled before E_UPDATE.
*/
void ClientApp::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;
/*    int key = eventData[P_KEY].GetInt();
    if(key == KEY_ESCAPE)
        engine_->Exit();
        */
}

void ClientApp::HandleKeyUp(StringHash eventType, VariantMap& eventData)
{

}

/**
* You can get these events from when ever the user interacts with the UI.
*/
void ClientApp::HandleClosePressed(StringHash eventType, VariantMap& eventData)
{
    engine_->Exit();
}

void ClientApp::SetWindowTitleAndIcon()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Graphics* graphics = GetSubsystem<Graphics>();
    Image* icon = cache->GetResource<Image>("Textures/Icon.png");
    graphics->SetWindowIcon(icon);
    graphics->SetWindowTitle("FW");
}

void ClientApp::SwitchScene(const String& sceneName)
{
    // Switch level
    VariantMap& eventData = GetEventDataMap();
    eventData[AbEvents::E_SET_LEVEL] = sceneName;
    SendEvent(AbEvents::E_SET_LEVEL, eventData);
}

/**
* Your non-rendering logic should be handled here.
* This could be moving objects, checking collisions and reaction, etc.
*/
void ClientApp::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[Update::P_TIMESTEP].GetFloat();
    framecount_++;
    time_ += timeStep;
    // Movement speed as world units per second
    float MOVE_SPEED = 10.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    if (time_ >= 1)
    {
        framecount_ = 0;
        time_ = 0;
    }

    Input* input = GetSubsystem<Input>();

    if (input->GetKeyPress(KEY_F11))
    {
        Options* options = GetSubsystem<Options>();
        options->SetFullscreen(!options->GetFullscreen());
    }
}

/**
* Anything in the non-rendering logic that requires a second pass,
* it might be well suited to be handled here.
*/
void ClientApp::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // We really don't have anything useful to do here for this example.
    // Probably shouldn't be subscribing to events we don't care about.
}

/**
* If you have any details you want to change before the viewport is
* rendered, try putting it here.
* See http://urho3d.github.io/documentation/1.32/_rendering.html
* for details on how the rendering pipeline is setup.
*/
void ClientApp::HandleRenderUpdate(StringHash eventType, VariantMap & eventData)
{
    // We really don't have anything useful to do here for this example.
    // Probably shouldn't be subscribing to events we don't care about.
}

/**
* After everything is rendered, there might still be things you wish
* to add to the rendering. At this point you cannot modify the scene,
* only post rendering is allowed. Good for adding things like debug
* artifacts on screen or brush up lighting, etc.
*/
void ClientApp::HandlePostRenderUpdate(StringHash eventType, VariantMap & eventData)
{
    // We could draw some debuggy looking thing for the octree.
    // scene_->GetComponent<Octree>()->DrawDebugGeometry(true);
}

/**
* All good things must come to an end.
*/
void ClientApp::HandleEndFrame(StringHash eventType, VariantMap& eventData)
{
    // We really don't have anything useful to do here for this example.
    // Probably shouldn't be subscribing to events we don't care about.
}
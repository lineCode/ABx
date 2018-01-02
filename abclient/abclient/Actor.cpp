// Most of it taken from the Character Demo

#include "stdafx.h"
#include "Actor.h"
#include "Definitions.h"
#include <SDL/SDL_timer.h>

#include <Urho3D/DebugNew.h>

Actor::Actor(Context* context) :
    GameObject(context),
    pickable_(false),
    castShadows_(true),
    mesh_(String::EMPTY),
    animController_(nullptr),
    model_(nullptr),
    selectedObject_(nullptr)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

Actor::~Actor()
{
}

void Actor::RegisterObject(Context* context)
{
    context->RegisterFactory<Actor>();
}

Actor* Actor::CreateActor(uint32_t id, Context* context, Scene* scene)
{
    Node* objectNode = scene->CreateChild();
    Actor* result = objectNode->CreateComponent<Actor>();
    result->id_ = id;

    Node* adjustNode = result->GetNode()->CreateChild("AdjNode");
    adjustNode->SetRotation(Quaternion(180, Vector3(0, 1, 0)));

    result->Init();
    adjustNode->CreateComponent<AnimationController>();
    result->animatedModel_ = adjustNode->CreateComponent<AnimatedModel>();
    result->animatedModel_->SetCastShadows(true);
    adjustNode->CreateComponent<AnimationController>();

    return result;

}

void Actor::Init()
{
    mesh_ = "/Models/Sphere.mdl";
    materials_.Push("/Materials/Stone.xml");
    if (!mesh_.Empty())
    {
        CreateModel();
    }
}

void Actor::CreateModel()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    RigidBody* body = GetNode()->CreateComponent<RigidBody>();
    // Set mass to zero we control Y pos
    body->SetMass(1.0f);
    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);
    body->SetLinearFactor(Vector3(1.0f, 0.0f, 1.0f));

    // spin node
    Node* adjustNode = GetNode()->GetChild("AdjNode");
    adjustNode->SetRotation(Quaternion(180, Vector3(0, 1, 0)));

    // Create the rendering component + animation controller
    if (type_ == Actor::Animated)
    {
        AnimatedModel* animModel = adjustNode->CreateComponent<AnimatedModel>();
        animModel->SetModel(cache->GetResource<Model>(mesh_));
        model_ = animModel;
        animController_ = adjustNode->CreateComponent<AnimationController>();
    }
    else
    {
        model_ = adjustNode->CreateComponent<StaticModel>();
        model_->SetModel(cache->GetResource<Model>(mesh_));
    }
    int i = 0;
    for (Vector<String>::ConstIterator it = materials_.Begin(); it != materials_.End(); it++, i++)
    {
        model_->SetMaterial(i, cache->GetResource<Material>((*it)));
    }
    model_->SetCastShadows(castShadows_);

    CollisionShape* shape = node_->CreateComponent<CollisionShape>();
    const BoundingBox& bb = model_->GetBoundingBox();
    shape->SetCylinder(bb.Size().x_, bb.Size().y_);
}

void Actor::FixedUpdate(float timeStep)
{
    const Vector3& cp = GetNode()->GetPosition();
    if (moveToPos_ != Vector3::ZERO && moveToPos_ != cp)
    {
        // Try to make moves smoother...

        // Difference between FixedUpdate time and network tick
        float networkDiff = (NETWORK_TICK_MS - timeStep);
        Vector3 pos = moveToPos_.Lerp(cp, networkDiff);
        GetNode()->SetPosition(pos);
    }
}

void Actor::MoveTo(const Vector3& newPos)
{
    moveToPos_ = newPos;
}

void Actor::Unserialize(PropReadStream& data)
{
    GameObject::Unserialize(data);
    std::string str;
    if (data.ReadString(str))
        name_ = String(str.data(), (unsigned)str.length());
}

void Actor::LoadXML(const XMLElement& source)
{
    assert(source.GetName() == "actor");

    String typeName = source.GetAttribute("type");
    if (typeName.Compare("Animated") == 0)
    {
        type_ = Actor::Animated;
    }
    else
    {
        type_ = Actor::Static;
    }
    if (source.HasAttribute("pickable"))
        pickable_ = source.GetBool("pickable");
    if (source.HasAttribute("castshadows"))
        castShadows_ = source.GetBool("castshadows");

    // Read the mesh
    XMLElement meshElem = source.GetChild("mesh");
    // We need a mesh
    assert(meshElem);
    mesh_ = meshElem.GetValue();

    // Get materials
    XMLElement materialsElem = source.GetChild("materials");
    if (materialsElem)
    {
        materials_.Clear();
        XMLElement matElem = materialsElem.GetChild("material");
        while (matElem)
        {
            materials_.Push(matElem.GetValue());
            matElem = matElem.GetNext("material");
        }
    }

    // If animated model, get animations if any
    if (type_ == Actor::Animated)
    {
        XMLElement animsElem = source.GetChild("animations");
        if (animsElem)
        {
            XMLElement aniElem = animsElem.GetChild("animation");
            while (aniElem)
            {
                String aniType = aniElem.GetAttribute("type");
                animations_[StringHash(aniType)] = aniElem.GetValue();

                aniElem = aniElem.GetNext("animation");
            }
        }
    }

    // Sounds
    XMLElement soundsElem = source.GetChild("sounds");
    if (soundsElem)
    {
        XMLElement soundElem = soundsElem.GetChild("sound");
        while (soundElem)
        {
            String soundType = soundElem.GetAttribute("type");
            sounds_[StringHash(soundType)] = soundElem.GetValue();
            soundElem = soundElem.GetNext("sound");
        }
    }
}

void Actor::PlaySoundEffect(SoundSource3D* soundSource, const StringHash& type, bool loop /* = false */)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    if (sounds_.Contains(type) && cache->Exists(sounds_[type]))
    {
        Sound* sound = cache->GetResource<Sound>(sounds_[type]);
        sound->SetLooped(loop);
        soundSource->Play(sound);
    }
    else if (type == SOUND_NONE && soundSource->IsPlaying())
    {
        soundSource->Stop();
    }
}
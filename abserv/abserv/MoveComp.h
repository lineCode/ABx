#pragma once

#include "Vector3.h"
#include <AB/ProtocolCodes.h>
#include "NetworkMessage.h"

namespace Game {

class Actor;

namespace Components {

class MoveComp
{
private:
    Actor& owner_;
    Math::Vector3 oldPosition_;
    float speedFactor_{ 1.0f };
    /// Move to moveDir_
    void UpdateMove(uint32_t timeElapsed);
    /// Turn to turnDir_
    void UpdateTurn(uint32_t timeElapsed);
    inline float GetSpeed(uint32_t timeElapsed, float baseSpeed)
    {
        return (static_cast<float>(timeElapsed) / baseSpeed) * speedFactor_;
    }
public:
    enum UpdateFlags : uint32_t
    {
        UpdateFlagMove = 1,
        UpdateFlagTurn = 1 << 1
    };
    MoveComp() = delete;
    explicit MoveComp(Actor& owner) :
        owner_(owner)
    { }
    // non-copyable
    MoveComp(const MoveComp&) = delete;
    MoveComp& operator=(const MoveComp&) = delete;
    ~MoveComp() = default;

    void Update(uint32_t timeElapsed, uint32_t flags);
    bool SetPosition(const Math::Vector3& pos);
    void HeadTo(const Math::Vector3& pos);
    /// Move in direction of rotation
    void Move(float speed, const Math::Vector3& amount);
    void Turn(float angle);
    void SetDirection(float worldAngle);
    float GetSpeedFactor() const
    {
        return speedFactor_;
    }
    void SetSpeedFactor(float value)
    {
        if (!Math::Equals(speedFactor_, value))
        {
            speedFactor_ = value;
            speedDirty_ = true;
        }
    }
    void AddSpeed(float value)
    {
        speedFactor_ += value;
        speedDirty_ = true;
    }

    const Math::Vector3& GetOldPosition() const
    {
        return oldPosition_;
    }
    bool IsMoving() const { return velocity_ != Math::Vector3::Zero; }
    void Write(Net::NetworkMessage& message);

    uint32_t moveDir_{ AB::GameProtocol::MoveDirectionNone };
    uint32_t turnDir_{ AB::GameProtocol::TurnDirectionNone };
    bool turned_{ false };
    bool moved_{ false };
    bool speedDirty_{ false };
    /// Manual direction set
    bool directionSet_{ false };
    bool newAngle_{ false };
    /// Sends a special message to the client to force the client to set the position.
    bool forcePosition_{ false };
    /// Velocity in Units/s.
    Math::Vector3 velocity_;
};

}
}

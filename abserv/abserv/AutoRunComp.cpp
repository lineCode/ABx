#include "stdafx.h"
#include "AutoRunComp.h"
#include "Creature.h"
#include "Game.h"
#include "MathUtils.h"

namespace Game {
namespace Components {

bool AutoRunComp::Follow(std::shared_ptr<GameObject> object)
{
    auto creature = object->GetThisDynamic<Creature>();
    if (!creature)
        return false;
    following_ = creature;
    maxDist_ = Creature::MAX_INTERACTION_DIST;
    if (auto f = following_.lock())
        return FindPath(f->transformation_.position_);
    return false;
}

bool AutoRunComp::Goto(const Math::Vector3& dest)
{
    maxDist_ = 0.2f;
    return FindPath(dest);
}

bool AutoRunComp::FindPath(const Math::Vector3& dest)
{
    destination_ = dest;
    static const Math::Vector3 EXTENDS(1.0f, 8.0f, 1.0f);

    const Math::Vector3& pos = owner_.transformation_.position_;
    if (pos.Distance(dest) < maxDist_)
        return false;

    std::vector<Math::Vector3> wp;
    bool succ = owner_.GetGame()->map_->FindPath(wp, pos,
        dest, EXTENDS);
#ifdef DEBUG_NAVIGATION
    std::stringstream ss;
    ss << "Goto from " << pos.ToString() <<
        " to " << dest.ToString() << " via " << wayPoints_.size() << " waypoints:";
    for (const auto& _wp : wayPoints_)
        ss << " " << _wp.ToString();
    LOG_DEBUG << ss.str() << std::endl;
#endif
    if (succ && wp.size() != 0)
    {
        wayPoints_ = wp;
        lastCalc_ = Utils::AbTick();
        return true;
    }
    lastCalc_ = 0;
    return false;
}

void AutoRunComp::Pop()
{
    wayPoints_.erase(wayPoints_.begin());
}

void AutoRunComp::MoveTo(uint32_t timeElapsed, const Math::Vector3& dest)
{
    const Math::Vector3& pos = owner_.transformation_.position_;
    float worldAngle = -Math::DegToRad(pos.AngleY(dest) - 180.0f);
    if (worldAngle < 0.0f)
        worldAngle += Math::M_PIF;
#ifdef DEBUG_NAVIGATION
    const float distance = dest.Distance(pos);
    LOG_DEBUG << "From " << pos.ToString() << " to " << dest.ToString() <<
        " angle " << worldAngle << " old angle " << Math::RadToDeg(owner_.transformation_.rotation_) <<
        ", distance " << distance << std::endl;
#endif
    if (fabs(owner_.transformation_.rotation_ - worldAngle) > 0.02f)
    {
        owner_.moveComp_.SetDirection(worldAngle);
    }
    owner_.moveComp_.Move(((float)(timeElapsed) / owner_.moveComp_.BaseSpeed) * owner_.GetActualMoveSpeed(),
        Math::Vector3::UnitZ);
}

void AutoRunComp::Update(uint32_t timeElapsed)
{
    if (!autoRun_)
        return;

    const Math::Vector3& pos = owner_.transformation_.position_;
    if (auto f = following_.lock())
    {
        if ((lastCalc_ != 0 && (Utils::AbTick() - lastCalc_) > 1000)
            && (destination_.Distance(f->transformation_.position_) > Creature::SWITCH_WAYPOINT_DIST))
        {
            // Find new path when following object moved and enough time passed
            FindPath(f->transformation_.position_);
        }
    }

    if (!HasWaypoints())
    {
        // Still auto running but no more waypoints, move close to dest
        const float distance = destination_.Distance(pos);
        // Remaining distance
        if (distance > maxDist_)
        {
            MoveTo(timeElapsed, destination_);
            return;
        }
        else
        {
            // If at dest reset
            owner_.stateComp_.SetState(AB::GameProtocol::CreatureStateIdle);
            autoRun_ = false;
        }
        return;
    }

    const Math::Vector3& pt = Next();
    const float distance = pt.Distance(pos);

    if (distance > Creature::SWITCH_WAYPOINT_DIST)
    {
        MoveTo(timeElapsed, pt);
    }
    else
    {
        // If we are close to this point remove it from the list
        Pop();
    }
}

}
}
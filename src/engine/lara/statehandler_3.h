#pragma once

#include "abstractstatehandler.h"
#include "engine/collisioninfo.h"
#include "hid/inputstate.h"

namespace engine
{
namespace lara
{
class StateHandler_3 final : public AbstractStateHandler
{
public:
    explicit StateHandler_3(LaraNode& lara)
        : AbstractStateHandler{lara, LaraStateId::JumpForward}
    {
    }

    void handleInput(CollisionInfo& /*collisionInfo*/) override
    {
        if(getGoalAnimState() == LaraStateId::SwandiveBegin || getGoalAnimState() == LaraStateId::Reach)
        {
            setGoalAnimState(LaraStateId::JumpForward);
        }

        if(getGoalAnimState() != LaraStateId::Death && getGoalAnimState() != LaraStateId::Stop)
        {
            if(getEngine().getInputHandler().getInputState().action && getHandStatus() == HandStatus::None)
            {
                setGoalAnimState(LaraStateId::Reach);
            }

            if(getEngine().getInputHandler().getInputState().moveSlow && getHandStatus() == HandStatus::None)
            {
                setGoalAnimState(LaraStateId::SwandiveBegin);
            }

            if(getLara().m_state.fallspeed > core::FreeFallSpeedThreshold)
            {
                setGoalAnimState(LaraStateId::FreeFall);
            }
        }

        if(getEngine().getInputHandler().getInputState().xMovement == hid::AxisMovement::Left)
        {
            subYRotationSpeed(2.25_deg, -3_deg);
        }
        else if(getEngine().getInputHandler().getInputState().xMovement == hid::AxisMovement::Right)
        {
            addYRotationSpeed(2.25_deg, 3_deg);
        }
    }

    void postprocessFrame(CollisionInfo& collisionInfo) override
    {
        collisionInfo.badPositiveDistance = core::HeightLimit;
        collisionInfo.badNegativeDistance = -core::ClimbLimit2ClickMin;
        collisionInfo.badCeilingDistance = 192_len;
        collisionInfo.facingAngle = getLara().m_state.rotation.Y;
        setMovementAngle(collisionInfo.facingAngle);
        collisionInfo.initHeightInfo(getLara().m_state.position.position, getEngine(), core::LaraWalkHeight);
        checkJumpWallSmash(collisionInfo);

        if(collisionInfo.mid.floorSpace.y > 0_len || getLara().m_state.fallspeed <= 0_spd)
        {
            return;
        }

        if(applyLandingDamage())
        {
            setGoalAnimState(LaraStateId::Death);
        }
        else if(getEngine().getInputHandler().getInputState().zMovement != hid::AxisMovement::Forward
                || getEngine().getInputHandler().getInputState().moveSlow)
        {
            setGoalAnimState(LaraStateId::Stop);
        }
        else
        {
            setGoalAnimState(LaraStateId::RunForward);
        }

        getLara().m_state.fallspeed = 0_spd;
        getLara().m_state.falling = false;
        getLara().m_state.speed = 0_spd;
        placeOnFloor(collisionInfo);

        laraUpdateImpl();
    }
};
} // namespace lara
} // namespace engine

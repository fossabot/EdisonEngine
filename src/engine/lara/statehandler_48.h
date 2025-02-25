#pragma once

#include "statehandler_onwater.h"

namespace engine
{
namespace lara
{
class StateHandler_48 final : public StateHandler_OnWater
{
public:
    explicit StateHandler_48(LaraNode& lara)
        : StateHandler_OnWater{lara, LaraStateId::OnWaterLeft}
    {
    }

    void handleInput(CollisionInfo& /*collisionInfo*/) override
    {
        if(getLara().m_state.health <= 0_hp)
        {
            setGoalAnimState(LaraStateId::WaterDeath);
            return;
        }

        setSwimToDiveKeypressDuration(0_frame);

        if(getEngine().getInputHandler().getInputState().xMovement == hid::AxisMovement::Left)
        {
            getLara().m_state.rotation.Y -= 2_deg;
        }
        else if(getEngine().getInputHandler().getInputState().xMovement == hid::AxisMovement::Right)
        {
            getLara().m_state.rotation.Y += 2_deg;
        }

        if(getEngine().getInputHandler().getInputState().stepMovement != hid::AxisMovement::Left)
        {
            setGoalAnimState(LaraStateId::OnWaterStop);
        }

        getLara().m_state.fallspeed = std::min(60_spd, getLara().m_state.fallspeed + 8_spd);
    }

    void postprocessFrame(CollisionInfo& collisionInfo) override
    {
        setMovementAngle(getLara().m_state.rotation.Y - 90_deg);
        commonOnWaterHandling(collisionInfo);
    }
};
} // namespace lara
} // namespace engine

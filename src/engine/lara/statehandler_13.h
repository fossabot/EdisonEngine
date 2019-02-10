#pragma once

#include "statehandler_underwater.h"

namespace engine
{
namespace lara
{
class StateHandler_13 final
        : public StateHandler_Underwater
{
public:
    explicit StateHandler_13(LaraNode& lara)
            : StateHandler_Underwater{lara, LaraStateId::UnderwaterStop}
    {
    }

    void handleInput(CollisionInfo& /*collisionInfo*/) override
    {
        if( getLara().m_state.health < 0_hp )
        {
            setGoalAnimState( LaraStateId::WaterDeath );
            return;
        }

        handleDiveRotationInput();

        if( getLevel().m_inputHandler->getInputState().jump )
        {
            setGoalAnimState( LaraStateId::UnderwaterForward );
        }

        getLara().m_state.fallspeed = std::max( 0_spd, getLara().m_state.fallspeed - 6_spd );
    }
};
}
}

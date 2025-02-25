#include "scionpiece.h"

#include "engine/laranode.h"
#include "hid/inputhandler.h"

namespace engine
{
namespace items
{
void ScionPieceItem::collide(LaraNode& lara, CollisionInfo& /*collisionInfo*/)
{
    static const InteractionLimits limits{
        core::BoundingBox{{-256_len, 540_len, -350_len}, {256_len, 740_len, -200_len}},
        {-10_deg, 0_deg, 0_deg},
        {10_deg, 0_deg, 0_deg}};

    m_state.rotation.X = 0_deg;
    m_state.rotation.Y = lara.m_state.rotation.Y;
    m_state.rotation.Z = 0_deg;

    if(!limits.canInteract(m_state, lara.m_state))
        return;

    if(lara.getCurrentAnimState() != LaraStateId::PickUp)
    {
        if(getEngine().getInputHandler().getInputState().action && lara.getHandStatus() == HandStatus::None
           && !lara.m_state.falling && lara.getCurrentAnimState() == LaraStateId::Stop)
        {
            lara.alignForInteraction({0_len, 640_len, -310_len}, m_state);
            lara.m_state.anim = getEngine().findAnimatedModelForType(TR1ItemId::AlternativeLara)->animations;
            lara.setCurrentAnimState(LaraStateId::PickUp);
            lara.setGoalAnimState(LaraStateId::PickUp);
            lara.m_state.frame_number = lara.m_state.anim->firstFrame;
            getEngine().getCameraController().setMode(CameraMode::Cinematic);
            lara.setHandStatus(HandStatus::Grabbing);
            getEngine().getCameraController().m_cinematicFrame = 0;
            getEngine().getCameraController().m_cinematicPos = lara.m_state.position.position;
            getEngine().getCameraController().m_cinematicRot = lara.m_state.rotation;
        }
    }
    else if(lara.m_state.frame_number == lara.m_state.anim->firstFrame + 44_frame)
    {
        m_state.triggerState = TriggerState::Invisible;
        getEngine().getInventory().put(m_state.type);
        setParent(getNode(), nullptr);
        m_state.collidable = false;
    }
}
} // namespace items
}
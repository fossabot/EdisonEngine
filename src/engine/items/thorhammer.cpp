#include "thorhammer.h"

#include "engine/laranode.h"

namespace engine
{
namespace items
{
ThorHammerHandle::ThorHammerHandle(const gsl::not_null<Engine*>& engine,
                                   const gsl::not_null<const loader::file::Room*>& room,
                                   const loader::file::Item& item,
                                   const loader::file::SkeletalModelType& animatedModel)
    : ModelItemNode{engine, room, item, true, animatedModel}
{
    m_block = engine->createItem<ThorHammerBlock>(TR1ItemId::ThorHammerBlock, room, item.rotation, item.position, 0);
    m_block->activate();
    m_block->m_state.triggerState = TriggerState::Active;
}

void ThorHammerHandle::update()
{
    switch(m_state.current_anim_state.get())
    {
    case 0:
        if(m_state.updateActivationTimeout())
        {
            m_state.goal_anim_state = 1_as;
        }
        else
        {
            deactivate();
            m_state.triggerState = TriggerState::Inactive;
        }
        break;
    case 1:
        if(m_state.updateActivationTimeout())
        {
            m_state.goal_anim_state = 2_as;
        }
        else
        {
            m_state.goal_anim_state = 0_as;
        }
        break;
    case 2:
        if(m_state.frame_number > m_state.anim->firstFrame + 30_frame)
        {
            auto posX = m_state.position.position.X;
            auto posZ = m_state.position.position.Z;
            if(m_state.rotation.Y == 0_deg)
            {
                posZ += 3 * core::SectorSize;
            }
            else if(m_state.rotation.Y == 90_deg)
            {
                posX += 3 * core::SectorSize;
            }
            else if(m_state.rotation.Y == 180_deg)
            {
                posZ -= 3 * core::SectorSize;
            }
            else if(m_state.rotation.Y == -90_deg)
            {
                posX -= 3 * core::SectorSize;
            }
            if(getEngine().getLara().m_state.health >= 0_hp)
            {
                if(posX - 520_len < getEngine().getLara().m_state.position.position.X
                   && posX + 520_len > getEngine().getLara().m_state.position.position.X
                   && posZ - 520_len < getEngine().getLara().m_state.position.position.Z
                   && posZ + 520_len > getEngine().getLara().m_state.position.position.Z)
                {
                    getEngine().getLara().m_state.health = -1_hp;
                    getEngine().getLara().m_state.anim
                        = &getEngine().findAnimatedModelForType(engine::TR1ItemId::Lara)->animations[139];
                    getEngine().getLara().m_state.frame_number = 3561_frame;
                    getEngine().getLara().setCurrentAnimState(LaraStateId::BoulderDeath);
                    getEngine().getLara().setGoalAnimState(LaraStateId::BoulderDeath);
                    getEngine().getLara().m_state.position.position.Y = m_state.position.position.Y;
                    getEngine().getLara().m_state.falling = false;
                }
            }
        }
        break;
    case 3:
    {
        const auto sector = loader::file::findRealFloorSector(m_state.position.position, m_state.position.room);
        const auto hi = HeightInfo::fromFloor(sector, m_state.position.position, getEngine().getItemNodes());
        getEngine().getLara().handleCommandSequence(hi.lastCommandSequenceOrDeath, true);

        const auto oldPosX = m_state.position.position.X;
        const auto oldPosZ = m_state.position.position.Z;
        if(m_state.rotation.Y == 0_deg)
        {
            m_state.position.position.Z += 3 * core::SectorSize;
        }
        else if(m_state.rotation.Y == 90_deg)
        {
            m_state.position.position.X += 3 * core::SectorSize;
        }
        else if(m_state.rotation.Y == 180_deg)
        {
            m_state.position.position.Z -= 3 * core::SectorSize;
        }
        else if(m_state.rotation.Y == -90_deg)
        {
            m_state.position.position.X -= 3 * core::SectorSize;
        }
        if(getEngine().getLara().m_state.health >= 0_hp)
        {
            m_state.position.room->patchHeightsForBlock(*this, -2 * core::SectorSize);
        }
        m_state.position.position.X = oldPosX;
        m_state.position.position.Z = oldPosZ;
        deactivate();
        m_state.triggerState = TriggerState::Deactivated;
        break;
    }
    default: break;
    }
    ModelItemNode::update();

    // sync anim
    const auto animIdx = std::distance(
        &getEngine().findAnimatedModelForType(engine::TR1ItemId::ThorHammerHandle)->animations[0], m_state.anim);
    m_block->m_state.anim
        = &getEngine().findAnimatedModelForType(engine::TR1ItemId::ThorHammerBlock)->animations[animIdx];
    m_block->m_state.frame_number = m_state.frame_number - m_state.anim->firstFrame + m_block->m_state.anim->firstFrame;
    m_block->m_state.current_anim_state = m_state.current_anim_state;
}

void ThorHammerHandle::collide(LaraNode& node, CollisionInfo& info)
{
    if(!info.policyFlags.is_set(CollisionInfo::PolicyFlags::EnableBaddiePush))
        return;

    if(!isNear(node, info.collisionRadius))
        return;

    enemyPush(node, info, false, true);
}

void ThorHammerBlock::collide(LaraNode& node, CollisionInfo& info)
{
    if(m_state.current_anim_state == 2_as)
        return;

    if(!info.policyFlags.is_set(CollisionInfo::PolicyFlags::EnableBaddiePush))
        return;

    if(!isNear(node, info.collisionRadius))
        return;

    enemyPush(node, info, false, true);
}
} // namespace items
} // namespace engine

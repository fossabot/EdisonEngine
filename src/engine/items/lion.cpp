#include "lion.h"

#include "engine/laranode.h"
#include "engine/particle.h"

namespace engine
{
namespace items
{
void Lion::update()
{
    if(m_state.triggerState == TriggerState::Invisible)
    {
        m_state.triggerState = TriggerState::Active;
    }

    m_state.initCreatureInfo(getEngine());

    core::Angle tiltRot = 0_deg;
    core::Angle angle = 0_deg;
    core::Angle headRot = 0_deg;

    if(m_state.health > 0_hp)
    {
        const ai::AiInfo aiInfo{getEngine(), m_state};
        if(aiInfo.ahead)
        {
            headRot = aiInfo.angle;
        }
        updateMood(getEngine(), m_state, aiInfo, true);
        angle = rotateTowardsTarget(m_state.creatureInfo->maximum_turn);
        switch(m_state.current_anim_state.get())
        {
        case 1:
            if(m_state.required_anim_state != 0_as)
            {
                m_state.goal_anim_state = m_state.required_anim_state;
            }
            else if(m_state.creatureInfo->mood != ai::Mood::Bored)
            {
                if(aiInfo.ahead && (m_state.touch_bits.to_ulong() & 0x380066UL))
                {
                    m_state.goal_anim_state = 7_as;
                }
                else if(aiInfo.ahead && aiInfo.distance < util::square(core::SectorSize))
                {
                    m_state.goal_anim_state = 4_as;
                }
                else
                {
                    m_state.goal_anim_state = 3_as;
                }
            }
            else
            {
                m_state.goal_anim_state = 2_as;
            }
            break;
        case 2:
            m_state.creatureInfo->maximum_turn = 2_deg;
            if(m_state.creatureInfo->mood != ai::Mood::Bored)
            {
                m_state.goal_anim_state = 1_as;
            }
            else if(util::rand15() < 128)
            {
                m_state.required_anim_state = 6_as;
                m_state.goal_anim_state = 1_as;
            }
            break;
        case 3:
            tiltRot = angle;
            m_state.creatureInfo->maximum_turn = 5_deg;
            if(m_state.creatureInfo->mood == ai::Mood::Bored)
            {
                m_state.goal_anim_state = 1_as;
            }
            else if(aiInfo.ahead && aiInfo.distance < util::square(core::SectorSize))
            {
                m_state.goal_anim_state = 1_as;
            }
            else if(aiInfo.ahead && (m_state.touch_bits.to_ulong() & 0x380066UL))
            {
                m_state.goal_anim_state = 1_as;
            }
            else if(m_state.creatureInfo->mood != ai::Mood::Escape && util::rand15() < 128)
            {
                m_state.required_anim_state = 6_as;
                m_state.goal_anim_state = 1_as;
            }
            break;
        case 4:
            if(m_state.required_anim_state == 0_as && (m_state.touch_bits.to_ulong() & 0x380066UL))
            {
                getEngine().getLara().m_state.health -= 150_hp;
                getEngine().getLara().m_state.is_hit = true;
                m_state.required_anim_state = 1_as;
            }
            break;
        case 7:
            if(m_state.required_anim_state == 0_as && (m_state.touch_bits.to_ulong() & 0x380066UL))
            {
                emitParticle({-2_len, -10_len, 132_len}, 21, &createBloodSplat);
                getEngine().getLara().m_state.health -= 250_hp;
                getEngine().getLara().m_state.is_hit = true;
                m_state.required_anim_state = 1_as;
            }
            break;
        default:
            // silence compiler
            break;
        }
    }
    else
    {
        if(m_state.current_anim_state != 5_as)
        {
            if(m_state.type == TR1ItemId::Panther)
            {
                m_state.anim
                    = &getEngine().findAnimatedModelForType(TR1ItemId::Panther)->animations[4 + util::rand15(2)];
            }
            else if(m_state.type == TR1ItemId::LionMale)
            {
                m_state.anim
                    = &getEngine().findAnimatedModelForType(TR1ItemId::LionMale)->animations[7 + util::rand15(2)];
                m_state.current_anim_state = 5_as;
                m_state.frame_number = m_state.anim->firstFrame;
            }
            else
            {
                m_state.anim
                    = &getEngine().findAnimatedModelForType(TR1ItemId::LionFemale)->animations[7 + util::rand15(2)];
                m_state.current_anim_state = 5_as;
                m_state.frame_number = m_state.anim->firstFrame;
            }
        }
    }

    rotateCreatureTilt(tiltRot);
    rotateCreatureHead(headRot);
    getSkeleton()->patchBone(20, core::TRRotation{0_deg, m_state.creatureInfo->head_rotation, 0_deg}.toMatrix());
    animateCreature(angle, tiltRot);
}
} // namespace items
} // namespace engine

#include "dart.h"

#include "engine/laranode.h"
#include "engine/particle.h"

namespace engine
{
namespace items
{
void Dart::collide(LaraNode& lara, CollisionInfo& info)
{
    if(!isNear(lara, info.collisionRadius))
        return;

    if(!testBoneCollision(lara))
        return;

    if(!info.policyFlags.is_set(CollisionInfo::PolicyFlags::EnableBaddiePush))
        return;

    enemyPush(lara, info, false, true);
}

void Dart::update()
{
    if(m_state.touch_bits != 0)
    {
        getEngine().getLara().m_state.health -= 50_hp;
        getEngine().getLara().m_state.is_hit = true;

        auto fx = createBloodSplat(getEngine(), m_state.position, m_state.speed, m_state.rotation.Y);
        getEngine().getParticles().emplace_back(fx);
    }

    ModelItemNode::update();

    auto room = m_state.position.room;
    const auto sector = loader::file::findRealFloorSector(m_state.position.position, &room);
    if(room != m_state.position.room)
        setCurrentRoom(room);

    const HeightInfo h = HeightInfo::fromFloor(sector, m_state.position.position, getEngine().getItemNodes());
    m_state.floor = h.y;

    if(m_state.position.position.Y < m_state.floor)
        return;

    kill();

    const auto particle = std::make_shared<RicochetParticle>(m_state.position, getEngine());
    setParent(particle, m_state.position.room->node);
    particle->angle = m_state.rotation;
    particle->timePerSpriteFrame = 6;
    getEngine().getParticles().emplace_back(particle);
}
} // namespace items
} // namespace engine

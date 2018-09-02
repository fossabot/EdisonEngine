#pragma once

#include "itemnode.h"

namespace engine
{
namespace items
{
class TeethSpikes final : public ModelItemNode
{
public:
    TeethSpikes(const gsl::not_null<level::Level*>& level,
              const std::string& name,
              const gsl::not_null<const loader::Room*>& room,
              const loader::Item& item,
              const loader::SkeletalModelType& animatedModel)
            : ModelItemNode( level, name, room, item, true, animatedModel )
    {
    }

    void collide(LaraNode& lara, CollisionInfo& collisionInfo) override;
};
}
}

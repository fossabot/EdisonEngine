#pragma once

#include "itemnode.h"

namespace engine
{
namespace items
{
class Dart final : public ModelItemNode
{
public:
    Dart(const gsl::not_null<Engine*>& engine,
         const gsl::not_null<const loader::file::Room*>& room,
         const loader::file::Item& item,
         const loader::file::SkeletalModelType& animatedModel)
        : ModelItemNode{engine, room, item, true, animatedModel}
    {
        m_state.collidable = true;
    }

    void collide(LaraNode& lara, CollisionInfo& info) override;

    void update() override;
};
} // namespace items
} // namespace engine

#pragma once

#include "itemnode.h"

namespace engine
{
namespace items
{
class SlopedBridge : public ModelItemNode
{
private:
    const int m_div;

public:
    SlopedBridge(const gsl::not_null<engine::Engine*>& engine,
                 const gsl::not_null<const loader::file::Room*>& room,
                 const loader::file::Item& item,
                 const loader::file::SkeletalModelType& animatedModel,
                 const int div)
        : ModelItemNode{engine, room, item, false, animatedModel}
        , m_div{div}
    {
    }

    void patchFloor(const core::TRVec& pos, core::Length& y) override final
    {
        const auto tmp = m_state.position.position.Y + getBridgeSlopeHeight(pos) / m_div;
        if(pos.Y <= tmp)
            y = tmp;
    }

    void patchCeiling(const core::TRVec& pos, core::Length& y) override final
    {
        const auto tmp = m_state.position.position.Y + getBridgeSlopeHeight(pos) / m_div;
        if(pos.Y <= tmp)
            return;

        y = tmp + core::QuarterSectorSize;
    }

private:
    core::Length getBridgeSlopeHeight(const core::TRVec& pos) const
    {
        auto axis = axisFromAngle(m_state.rotation.Y, 1_deg);
        Expects(axis.is_initialized());

        switch(*axis)
        {
        case core::Axis::PosZ: return core::SectorSize - 1_len - pos.X % core::SectorSize;
        case core::Axis::PosX: return pos.Z % core::SectorSize;
        case core::Axis::NegZ: return pos.X % core::SectorSize;
        case core::Axis::NegX: return core::SectorSize - 1_len - pos.Z % core::SectorSize;
        default: return 0_len;
        }
    }
};

class BridgeSlope1 final : public SlopedBridge
{
public:
    BridgeSlope1(const gsl::not_null<engine::Engine*>& engine,
                 const gsl::not_null<const loader::file::Room*>& room,
                 const loader::file::Item& item,
                 const loader::file::SkeletalModelType& animatedModel)
        : SlopedBridge{engine, room, item, animatedModel, 4}
    {
    }
};

class BridgeSlope2 final : public SlopedBridge
{
public:
    BridgeSlope2(const gsl::not_null<engine::Engine*>& engine,
                 const gsl::not_null<const loader::file::Room*>& room,
                 const loader::file::Item& item,
                 const loader::file::SkeletalModelType& animatedModel)
        : SlopedBridge{engine, room, item, animatedModel, 2}
    {
    }
};
} // namespace items
} // namespace engine

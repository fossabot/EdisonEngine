#include "abstractstatehandler.h"

#include "statehandler_0.h"
#include "statehandler_1.h"
#include "statehandler_10.h"
#include "statehandler_11.h"
#include "statehandler_12.h"
#include "statehandler_13.h"
#include "statehandler_14.h"
#include "statehandler_15.h"
#include "statehandler_16.h"
#include "statehandler_17.h"
#include "statehandler_18.h"
#include "statehandler_19.h"
#include "statehandler_2.h"
#include "statehandler_20.h"
#include "statehandler_21.h"
#include "statehandler_22.h"
#include "statehandler_23.h"
#include "statehandler_24.h"
#include "statehandler_25.h"
#include "statehandler_26.h"
#include "statehandler_27.h"
#include "statehandler_28.h"
#include "statehandler_29.h"
#include "statehandler_3.h"
#include "statehandler_30.h"
#include "statehandler_31.h"
#include "statehandler_32.h"
#include "statehandler_33.h"
#include "statehandler_34.h"
#include "statehandler_35.h"
#include "statehandler_36.h"
#include "statehandler_37.h"
#include "statehandler_38.h"
#include "statehandler_39.h"
#include "statehandler_4.h"
#include "statehandler_40.h"
#include "statehandler_41.h"
#include "statehandler_42.h"
#include "statehandler_43.h"
#include "statehandler_44.h"
#include "statehandler_45.h"
#include "statehandler_46.h"
#include "statehandler_47.h"
#include "statehandler_48.h"
#include "statehandler_49.h"
#include "statehandler_5.h"
#include "statehandler_50.h"
#include "statehandler_51.h"
#include "statehandler_52.h"
#include "statehandler_53.h"
#include "statehandler_54.h"
#include "statehandler_55.h"
#include "statehandler_6.h"
#include "statehandler_7.h"
#include "statehandler_8.h"
#include "statehandler_9.h"

#include <cstdlib>

namespace engine
{
namespace lara
{
std::unique_ptr<AbstractStateHandler> AbstractStateHandler::create(const LaraStateId id, LaraNode& lara)
{
    switch(id)
    {
    case LaraStateId::WalkForward: return std::make_unique<StateHandler_0>(lara);
    case LaraStateId::RunForward: return std::make_unique<StateHandler_1>(lara);
    case LaraStateId::Stop: return std::make_unique<StateHandler_2>(lara);
    case LaraStateId::JumpForward: return std::make_unique<StateHandler_3>(lara);
    case LaraStateId::Pose: return std::make_unique<StateHandler_4>(lara);
    case LaraStateId::RunBack: return std::make_unique<StateHandler_5>(lara);
    case LaraStateId::TurnRightSlow: return std::make_unique<StateHandler_6>(lara);
    case LaraStateId::TurnLeftSlow: return std::make_unique<StateHandler_7>(lara);
    case LaraStateId::Death: return std::make_unique<StateHandler_8>(lara);
    case LaraStateId::FreeFall: return std::make_unique<StateHandler_9>(lara);
    case LaraStateId::Hang: return std::make_unique<StateHandler_10>(lara);
    case LaraStateId::Reach: return std::make_unique<StateHandler_11>(lara);
    case LaraStateId::Unknown12: return std::make_unique<StateHandler_12>(lara);
    case LaraStateId::UnderwaterStop: return std::make_unique<StateHandler_13>(lara);
    case LaraStateId::GrabToFall: return std::make_unique<StateHandler_14>(lara);
    case LaraStateId::JumpPrepare: return std::make_unique<StateHandler_15>(lara);
    case LaraStateId::WalkBackward: return std::make_unique<StateHandler_16>(lara);
    case LaraStateId::UnderwaterForward: return std::make_unique<StateHandler_17>(lara);
    case LaraStateId::UnderwaterInertia: return std::make_unique<StateHandler_18>(lara);
    case LaraStateId::Climbing: return std::make_unique<StateHandler_19>(lara);
    case LaraStateId::TurnFast: return std::make_unique<StateHandler_20>(lara);
    case LaraStateId::StepRight: return std::make_unique<StateHandler_21>(lara);
    case LaraStateId::StepLeft: return std::make_unique<StateHandler_22>(lara);
    case LaraStateId::RollBackward: return std::make_unique<StateHandler_23>(lara);
    case LaraStateId::SlideForward: return std::make_unique<StateHandler_24>(lara);
    case LaraStateId::JumpBack: return std::make_unique<StateHandler_25>(lara);
    case LaraStateId::JumpLeft: return std::make_unique<StateHandler_26>(lara);
    case LaraStateId::JumpRight: return std::make_unique<StateHandler_27>(lara);
    case LaraStateId::JumpUp: return std::make_unique<StateHandler_28>(lara);
    case LaraStateId::FallBackward: return std::make_unique<StateHandler_29>(lara);
    case LaraStateId::ShimmyLeft: return std::make_unique<StateHandler_30>(lara);
    case LaraStateId::ShimmyRight: return std::make_unique<StateHandler_31>(lara);
    case LaraStateId::SlideBackward: return std::make_unique<StateHandler_32>(lara);
    case LaraStateId::OnWaterStop: return std::make_unique<StateHandler_33>(lara);
    case LaraStateId::OnWaterForward: return std::make_unique<StateHandler_34>(lara);
    case LaraStateId::UnderwaterDiving: return std::make_unique<StateHandler_35>(lara);
    case LaraStateId::PushablePush: return std::make_unique<StateHandler_36>(lara);
    case LaraStateId::PushablePull: return std::make_unique<StateHandler_37>(lara);
    case LaraStateId::PushableGrab: return std::make_unique<StateHandler_38>(lara);
    case LaraStateId::PickUp: return std::make_unique<StateHandler_39>(lara);
    case LaraStateId::SwitchDown: return std::make_unique<StateHandler_40>(lara);
    case LaraStateId::SwitchUp: return std::make_unique<StateHandler_41>(lara);
    case LaraStateId::InsertKey: return std::make_unique<StateHandler_42>(lara);
    case LaraStateId::InsertPuzzle: return std::make_unique<StateHandler_43>(lara);
    case LaraStateId::WaterDeath: return std::make_unique<StateHandler_44>(lara);
    case LaraStateId::RollForward: return std::make_unique<StateHandler_45>(lara);
    case LaraStateId::BoulderDeath: return std::make_unique<StateHandler_46>(lara);
    case LaraStateId::OnWaterBackward: return std::make_unique<StateHandler_47>(lara);
    case LaraStateId::OnWaterLeft: return std::make_unique<StateHandler_48>(lara);
    case LaraStateId::OnWaterRight: return std::make_unique<StateHandler_49>(lara);
    case LaraStateId::UseMidas: return std::make_unique<StateHandler_50>(lara);
    case LaraStateId::MidasDeath: return std::make_unique<StateHandler_51>(lara);
    case LaraStateId::SwandiveBegin: return std::make_unique<StateHandler_52>(lara);
    case LaraStateId::SwandiveEnd: return std::make_unique<StateHandler_53>(lara);
    case LaraStateId::Handstand: return std::make_unique<StateHandler_54>(lara);
    case LaraStateId::OnWaterExit: return std::make_unique<StateHandler_55>(lara);
    default:
        BOOST_LOG_TRIVIAL(error) << "No state handler for state " << toString(id);
        BOOST_THROW_EXCEPTION(std::runtime_error("Unhandled state"));
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setAir(const core::Frame a) noexcept
{
    m_lara.setAir(a);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setMovementAngle(const core::Angle angle) noexcept
{
    m_lara.setMovementAngle(angle);
}

core::Angle AbstractStateHandler::getMovementAngle() const noexcept
{
    return m_lara.getMovementAngle();
}

HandStatus AbstractStateHandler::getHandStatus() const noexcept
{
    return m_lara.getHandStatus();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setHandStatus(const HandStatus status) noexcept
{
    m_lara.setHandStatus(status);
}

LaraStateId AbstractStateHandler::getCurrentAnimState() const
{
    return m_lara.getCurrentAnimState();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setAnimation(const AnimationId anim, const boost::optional<core::Frame>& firstFrame)
{
    m_lara.setAnimation(anim, firstFrame);
    m_lara.drawRoutine();
}

const Engine& AbstractStateHandler::getEngine() const
{
    return m_lara.getEngine();
}

Engine& AbstractStateHandler::getEngine()
{
    return m_lara.getEngine();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::placeOnFloor(const CollisionInfo& collisionInfo)
{
    m_lara.placeOnFloor(collisionInfo);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setYRotationSpeed(const core::Angle spd)
{
    m_lara.setYRotationSpeed(spd);
}

core::Angle AbstractStateHandler::getYRotationSpeed() const
{
    return m_lara.getYRotationSpeed();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::subYRotationSpeed(const core::Angle val, const core::Angle limit)
{
    m_lara.subYRotationSpeed(val, limit);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::addYRotationSpeed(const core::Angle val, const core::Angle limit)
{
    m_lara.addYRotationSpeed(val, limit);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setFallSpeedOverride(const core::Speed v)
{
    m_lara.setFallSpeedOverride(v);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::dampenHorizontalSpeed(const float f)
{
    m_lara.dampenHorizontalSpeed(f);
}

core::Angle AbstractStateHandler::getCurrentSlideAngle() const noexcept
{
    return m_lara.getCurrentSlideAngle();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setCurrentSlideAngle(const core::Angle a) noexcept
{
    m_lara.setCurrentSlideAngle(a);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setGoalAnimState(const LaraStateId state)
{
    m_lara.setGoalAnimState(state);
}

LaraStateId AbstractStateHandler::getGoalAnimState() const
{
    return m_lara.getGoalAnimState();
}

bool AbstractStateHandler::canClimbOnto(const core::Axis axis) const
{
    auto pos = m_lara.m_state.position.position;
    switch(axis)
    {
    case core::Axis::PosZ: pos.Z += core::QuarterSectorSize; break;
    case core::Axis::PosX: pos.X += core::QuarterSectorSize; break;
    case core::Axis::NegZ: pos.Z -= core::QuarterSectorSize; break;
    case core::Axis::NegX: pos.X -= core::QuarterSectorSize; break;
    }

    const auto sector = findRealFloorSector(pos, m_lara.m_state.position.room);
    VerticalSpaceInfo space;
    space.init(sector, pos, getEngine().getItemNodes(), pos.Y, 400_len);
    return space.floorSpace.y != -core::HeightLimit && space.floorSpace.y > 0_len && space.ceilingSpace.y < 0_len;
}

bool AbstractStateHandler::tryReach(CollisionInfo& collisionInfo)
{
    if(collisionInfo.collisionType != CollisionInfo::AxisColl::Front
       || !getEngine().getInputHandler().getInputState().action || getHandStatus() != HandStatus::None)
    {
        return false;
    }

    if(abs(collisionInfo.frontLeft.floorSpace.y - collisionInfo.frontRight.floorSpace.y) >= core::MaxGrabbableGradient)
    {
        return false;
    }

    if(collisionInfo.front.ceilingSpace.y > 0_len || collisionInfo.mid.ceilingSpace.y > -core::ClimbLimit2ClickMin
       || collisionInfo.mid.floorSpace.y < 200_len)
    { //! @todo Magick 200
        return false;
    }

    const auto bbox = getBoundingBox();
    const auto spaceToReach = collisionInfo.front.floorSpace.y - bbox.minY;

    if(spaceToReach < 0_len && m_lara.m_state.fallspeed * 1_frame + spaceToReach < 0_len)
    {
        return false;
    }
    if(spaceToReach > 0_len && m_lara.m_state.fallspeed * 1_frame + spaceToReach > 0_len)
    {
        return false;
    }

    auto alignedRotation = alignRotation(m_lara.m_state.rotation.Y, 35_deg);
    if(!alignedRotation)
    {
        return false;
    }

    if(canClimbOnto(*axisFromAngle(m_lara.m_state.rotation.Y, 35_deg)))
    {
        setAnimation(AnimationId::OSCILLATE_HANG_ON, 3974_frame);
    }
    else
    {
        setAnimation(AnimationId::HANG_IDLE, 1493_frame);
    }

    setGoalAnimState(LaraStateId::Hang);
    const core::TRVec& pos
        = m_lara.m_state.position.position + core::TRVec(collisionInfo.shift.X, spaceToReach, collisionInfo.shift.Z);
    m_lara.m_state.position.position = pos;
    m_lara.m_state.speed = 0_spd;
    m_lara.m_state.rotation.Y = *alignedRotation;
    m_lara.m_state.falling = false;
    m_lara.m_state.fallspeed = 0_spd;
    setHandStatus(HandStatus::Grabbing);
    return true;
}

bool AbstractStateHandler::stopIfCeilingBlocked(const CollisionInfo& collisionInfo)
{
    if(collisionInfo.collisionType != CollisionInfo::AxisColl::Top
       && collisionInfo.collisionType != CollisionInfo::AxisColl::TopFront)
    {
        return false;
    }

    m_lara.m_state.position.position = collisionInfo.oldPosition;

    setGoalAnimState(LaraStateId::Stop);
    setAnimation(AnimationId::STAY_SOLID, 185_frame);
    m_lara.m_state.speed = 0_spd;
    m_lara.m_state.fallspeed = 0_spd;
    m_lara.m_state.falling = false;
    return true;
}

bool AbstractStateHandler::tryClimb(CollisionInfo& collisionInfo)
{
    if(collisionInfo.collisionType != CollisionInfo::AxisColl::Front
       || !getEngine().getInputHandler().getInputState().action || getHandStatus() != HandStatus::None)
    {
        return false;
    }

    const auto floorGradient = abs(collisionInfo.frontLeft.floorSpace.y - collisionInfo.frontRight.floorSpace.y);
    if(floorGradient >= core::MaxGrabbableGradient)
    {
        return false;
    }

    //! @todo MAGICK +/- 30 degrees
    auto alignedRotation = alignRotation(m_lara.m_state.rotation.Y, 30_deg);
    if(!alignedRotation)
    {
        return false;
    }

    const auto climbHeight = collisionInfo.front.floorSpace.y;
    if(climbHeight >= -core::ClimbLimit2ClickMax && climbHeight <= -core::ClimbLimit2ClickMin)
    {
        if(climbHeight < collisionInfo.front.ceilingSpace.y
           || collisionInfo.frontLeft.floorSpace.y < collisionInfo.frontLeft.ceilingSpace.y
           || collisionInfo.frontRight.floorSpace.y < collisionInfo.frontRight.ceilingSpace.y)
        {
            return false;
        }

        setGoalAnimState(LaraStateId::Stop);
        setAnimation(AnimationId::CLIMB_2CLICK, 759_frame);
        m_lara.m_state.position.position.Y += 2 * core::QuarterSectorSize + climbHeight;
        setHandStatus(HandStatus::Grabbing);
    }
    else if(climbHeight >= -core::ClimbLimit3ClickMax && climbHeight <= -core::ClimbLimit2ClickMax)
    {
        if(collisionInfo.front.floorSpace.y < collisionInfo.front.ceilingSpace.y
           || collisionInfo.frontLeft.floorSpace.y < collisionInfo.frontLeft.ceilingSpace.y
           || collisionInfo.frontRight.floorSpace.y < collisionInfo.frontRight.ceilingSpace.y)
        {
            return false;
        }

        setGoalAnimState(LaraStateId::Stop);
        setAnimation(AnimationId::CLIMB_3CLICK, 614_frame);
        m_lara.m_state.position.position.Y += 3 * core::QuarterSectorSize + climbHeight;
        setHandStatus(HandStatus::Grabbing);
    }
    else if(climbHeight >= -core::JumpReachableHeight && climbHeight <= -core::ClimbLimit3ClickMax)
    {
        setGoalAnimState(LaraStateId::JumpUp);
        setAnimation(AnimationId::STAY_SOLID, 185_frame);
        setFallSpeedOverride(-(sqrt(-core::Acceleration{12} * (climbHeight + 800_len)) + 3_spd));
        m_lara.updateImpl();
    }
    else
    {
        return false;
    }

    m_lara.m_state.rotation.Y = *alignedRotation;
    applyShift(collisionInfo);

    return true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::applyShift(const CollisionInfo& collisionInfo)
{
    m_lara.applyShift(collisionInfo);
}

bool AbstractStateHandler::checkWallCollision(CollisionInfo& collisionInfo)
{
    if(collisionInfo.collisionType == CollisionInfo::AxisColl::Front
       || collisionInfo.collisionType == CollisionInfo::AxisColl::TopBottom)
    {
        applyShift(collisionInfo);
        setGoalAnimState(LaraStateId::Stop);
        m_lara.m_state.falling = false;
        m_lara.m_state.speed = 0_spd;
        return true;
    }

    if(collisionInfo.collisionType == CollisionInfo::AxisColl::Left)
    {
        applyShift(collisionInfo);
        m_lara.m_state.rotation.Y += 5_deg;
    }
    else if(collisionInfo.collisionType == CollisionInfo::AxisColl::Right)
    {
        applyShift(collisionInfo);
        m_lara.m_state.rotation.Y -= 5_deg;
    }

    return false;
}

bool AbstractStateHandler::tryStartSlide(const CollisionInfo& collisionInfo)
{
    const auto slantX = std::labs(collisionInfo.floorSlantX);
    const auto slantZ = std::labs(collisionInfo.floorSlantZ);
    if(slantX <= 2 && slantZ <= 2)
    {
        return false;
    }

    core::Angle targetAngle{0_deg};
    if(collisionInfo.floorSlantX < -2)
    {
        targetAngle = 90_deg;
    }
    else if(collisionInfo.floorSlantX > 2)
    {
        targetAngle = -90_deg;
    }

    if(collisionInfo.floorSlantZ > std::max(2L, slantX))
    {
        targetAngle = 180_deg;
    }
    else if(collisionInfo.floorSlantZ < std::min(-2L, -slantX))
    {
        targetAngle = 0_deg;
    }

    const core::Angle dy = abs(targetAngle - m_lara.m_state.rotation.Y);
    applyShift(collisionInfo);
    if(dy > 90_deg || dy < -90_deg)
    {
        if(getCurrentAnimState() != LaraStateId::SlideBackward || targetAngle != getCurrentSlideAngle())
        {
            setAnimation(AnimationId::START_SLIDE_BACKWARD, 1677_frame);
            setGoalAnimState(LaraStateId::SlideBackward);
            setMovementAngle(targetAngle);
            setCurrentSlideAngle(targetAngle);
            m_lara.m_state.rotation.Y = targetAngle - 180_deg;
        }
    }
    else if(getCurrentAnimState() != LaraStateId::SlideForward || targetAngle != getCurrentSlideAngle())
    {
        setAnimation(AnimationId::SLIDE_FORWARD, 1133_frame);
        setGoalAnimState(LaraStateId::SlideForward);
        setMovementAngle(targetAngle);
        setCurrentSlideAngle(targetAngle);
        m_lara.m_state.rotation.Y = targetAngle;
    }
    return true;
}

bool AbstractStateHandler::tryGrabEdge(CollisionInfo& collisionInfo)
{
    if(collisionInfo.collisionType != CollisionInfo::AxisColl::Front
       || !getEngine().getInputHandler().getInputState().action || getHandStatus() != HandStatus::None)
    {
        return false;
    }

    const auto floorGradient = abs(collisionInfo.frontLeft.floorSpace.y - collisionInfo.frontRight.floorSpace.y);
    if(floorGradient >= core::MaxGrabbableGradient)
    {
        return false;
    }

    if(collisionInfo.front.ceilingSpace.y > 0_len || collisionInfo.mid.ceilingSpace.y > -core::ClimbLimit2ClickMin)
    {
        return false;
    }

    auto bbox = getBoundingBox();
    auto spaceToReach = collisionInfo.front.floorSpace.y - bbox.minY;

    if(spaceToReach < 0_len && m_lara.m_state.fallspeed * 1_frame + spaceToReach < 0_len)
    {
        return false;
    }
    if(spaceToReach > 0_len && m_lara.m_state.fallspeed * 1_frame + spaceToReach > 0_len)
    {
        return false;
    }

    auto alignedRotation = alignRotation(m_lara.m_state.rotation.Y, 35_deg);
    if(!alignedRotation)
    {
        return false;
    }

    setGoalAnimState(LaraStateId::Hang);
    setAnimation(AnimationId::HANG_IDLE, 1505_frame);
    bbox = getBoundingBox();
    spaceToReach = collisionInfo.front.floorSpace.y - bbox.minY;

    const core::TRVec pos = m_lara.m_state.position.position + core::TRVec(0_len, spaceToReach, 0_len);
    m_lara.m_state.position.position = pos;
    applyShift(collisionInfo);
    m_lara.m_state.speed = 0_spd;
    m_lara.m_state.fallspeed = 0_spd;
    m_lara.m_state.falling = false;
    setHandStatus(HandStatus::Grabbing);
    m_lara.m_state.rotation.Y = *alignedRotation;

    return true;
}

core::Length AbstractStateHandler::getRelativeHeightAtDirection(core::Angle angle, const core::Length dist) const
{
    auto pos = m_lara.m_state.position.position + util::pitch(dist, angle);
    pos.Y -= core::LaraWalkHeight;

    const auto sector = findRealFloorSector(pos, m_lara.m_state.position.room);

    HeightInfo h = HeightInfo::fromFloor(sector, pos, getEngine().getItemNodes());

    if(h.y != -core::HeightLimit)
    {
        h.y -= m_lara.m_state.position.position.Y;
    }

    return h.y;
}

void AbstractStateHandler::commonJumpHandling(CollisionInfo& collisionInfo)
{
    collisionInfo.badPositiveDistance = core::HeightLimit;
    collisionInfo.badNegativeDistance = -core::ClimbLimit2ClickMin;
    collisionInfo.badCeilingDistance = 192_len;
    collisionInfo.facingAngle = getMovementAngle();
    collisionInfo.initHeightInfo(m_lara.m_state.position.position, getEngine(), core::LaraWalkHeight);
    checkJumpWallSmash(collisionInfo);
    if(m_lara.m_state.fallspeed <= 0_spd || collisionInfo.mid.floorSpace.y > 0_len)
    {
        return;
    }

    if(applyLandingDamage())
    {
        setGoalAnimState(LaraStateId::Death);
    }
    else
    {
        setGoalAnimState(LaraStateId::Stop);
    }
    m_lara.m_state.fallspeed = 0_spd;
    placeOnFloor(collisionInfo);
    m_lara.m_state.falling = false;
}

void AbstractStateHandler::commonSlideHandling(CollisionInfo& collisionInfo)
{
    collisionInfo.badPositiveDistance = core::HeightLimit;
    collisionInfo.badNegativeDistance = -core::QuarterSectorSize * 2;
    collisionInfo.badCeilingDistance = 0_len;
    collisionInfo.facingAngle = getMovementAngle();
    collisionInfo.initHeightInfo(m_lara.m_state.position.position, getEngine(), core::LaraWalkHeight);

    if(stopIfCeilingBlocked(collisionInfo))
    {
        return;
    }

    checkWallCollision(collisionInfo);
    if(collisionInfo.mid.floorSpace.y <= 200_len)
    {
        tryStartSlide(collisionInfo);
        placeOnFloor(collisionInfo);
        if(std::labs(collisionInfo.floorSlantX) <= 2 && std::labs(collisionInfo.floorSlantZ) <= 2)
        {
            setGoalAnimState(LaraStateId::Stop);
        }
        return;
    }

    if(getCurrentAnimState() == LaraStateId::SlideForward)
    {
        setAnimation(AnimationId::FREE_FALL_FORWARD, 492_frame);
        setGoalAnimState(LaraStateId::JumpForward);
    }
    else
    {
        setAnimation(AnimationId::FREE_FALL_BACK, 1473_frame);
        setGoalAnimState(LaraStateId::FallBackward);
    }

    m_lara.m_state.fallspeed = 0_spd;
    m_lara.m_state.falling = true;
}

void AbstractStateHandler::commonEdgeHangHandling(CollisionInfo& collisionInfo)
{
    collisionInfo.badPositiveDistance = core::HeightLimit;
    collisionInfo.badNegativeDistance = -core::HeightLimit;
    collisionInfo.badCeilingDistance = 0_len;
    collisionInfo.facingAngle = getMovementAngle();
    collisionInfo.initHeightInfo(m_lara.m_state.position.position, getEngine(), core::LaraWalkHeight);
    const bool tooSteepToGrab = collisionInfo.front.floorSpace.y < 200_len;
    m_lara.m_state.fallspeed = 0_spd;
    m_lara.m_state.falling = false;
    setMovementAngle(m_lara.m_state.rotation.Y);
    const auto axis = *axisFromAngle(getMovementAngle(), 45_deg);
    switch(axis)
    {
    case core::Axis::PosZ:
    {
        const core::TRVec& pos = m_lara.m_state.position.position + core::TRVec(0_len, 0_len, 2_len);
        m_lara.m_state.position.position = pos;
    }
    break;
    case core::Axis::PosX:
    {
        const core::TRVec& pos = m_lara.m_state.position.position + core::TRVec(2_len, 0_len, 0_len);
        m_lara.m_state.position.position = pos;
    }
    break;
    case core::Axis::NegZ:
    {
        const core::TRVec& pos = m_lara.m_state.position.position - core::TRVec(0_len, 0_len, 2_len);
        m_lara.m_state.position.position = pos;
    }
    break;
    case core::Axis::NegX:
    {
        const core::TRVec& pos = m_lara.m_state.position.position - core::TRVec(2_len, 0_len, 0_len);
        m_lara.m_state.position.position = pos;
    }
    break;
    }

    collisionInfo.badPositiveDistance = core::HeightLimit;
    collisionInfo.badNegativeDistance = -core::ClimbLimit2ClickMin;
    collisionInfo.badCeilingDistance = 0_len;
    collisionInfo.facingAngle = getMovementAngle();
    collisionInfo.initHeightInfo(m_lara.m_state.position.position, getEngine(), core::LaraWalkHeight);
    if(!getEngine().getInputHandler().getInputState().action || m_lara.m_state.health <= 0_hp)
    {
        setAnimation(AnimationId::TRY_HANG_VERTICAL, 448_frame);
        setGoalAnimState(LaraStateId::JumpUp);
        setHandStatus(HandStatus::None);
        const auto bbox = getBoundingBox();
        const auto hangDistance = collisionInfo.front.floorSpace.y - bbox.minY + 2_len;
        const core::TRVec& pos = m_lara.m_state.position.position
                                 + core::TRVec(collisionInfo.shift.X, hangDistance, collisionInfo.shift.Z);
        m_lara.m_state.position.position = pos;
        m_lara.m_state.speed = 2_spd;
        m_lara.m_state.fallspeed = 1_spd;
        m_lara.m_state.falling = true;
        return;
    }

    const auto gradient = abs(collisionInfo.frontLeft.floorSpace.y - collisionInfo.frontRight.floorSpace.y);
    if(gradient >= core::MaxGrabbableGradient || collisionInfo.mid.ceilingSpace.y >= 0_len
       || collisionInfo.collisionType != CollisionInfo::AxisColl::Front || tooSteepToGrab)
    {
        m_lara.m_state.position.position = collisionInfo.oldPosition;
        if(getCurrentAnimState() != LaraStateId::ShimmyLeft && getCurrentAnimState() != LaraStateId::ShimmyRight)
        {
            return;
        }

        setAnimation(AnimationId::HANG_IDLE, 1514_frame);
        setGoalAnimState(LaraStateId::Hang);
        return;
    }

    switch(axis)
    {
    case core::Axis::PosZ:
    case core::Axis::NegZ:
    {
        const core::TRVec& pos = m_lara.m_state.position.position + core::TRVec(0_len, 0_len, collisionInfo.shift.Z);
        m_lara.m_state.position.position = pos;
    }
    break;
    case core::Axis::PosX:
    case core::Axis::NegX:
    {
        const core::TRVec& pos = m_lara.m_state.position.position + core::TRVec(collisionInfo.shift.X, 0_len, 0_len);
        m_lara.m_state.position.position = pos;
    }
    break;
    }

    const auto bbox = getBoundingBox();
    const auto spaceToReach = collisionInfo.front.floorSpace.y - bbox.minY;

    if(spaceToReach >= -core::QuarterSectorSize && spaceToReach <= core::QuarterSectorSize)
    {
        const core::TRVec& pos = m_lara.m_state.position.position + core::TRVec(0_len, spaceToReach, 0_len);
        m_lara.m_state.position.position = pos;
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
bool AbstractStateHandler::applyLandingDamage()
{
    const auto sector = findRealFloorSector(m_lara.m_state.position.position, m_lara.m_state.position.room);
    const HeightInfo h
        = HeightInfo::fromFloor(sector,
                                m_lara.m_state.position.position - core::TRVec{0_len, core::LaraWalkHeight, 0_len},
                                getEngine().getItemNodes());
    m_lara.m_state.floor = h.y;
    m_lara.handleCommandSequence(h.lastCommandSequenceOrDeath, false);
    const auto damageSpeed = m_lara.m_state.fallspeed - core::DamageFallSpeedThreshold;
    if(damageSpeed <= 0_spd)
    {
        return false;
    }

    static constexpr auto DeathSpeedLimit = core::DeadlyFallSpeedThreshold - core::DamageFallSpeedThreshold;
    static constexpr auto DeathSpeedLimitSq = util::square(DeathSpeedLimit);

    if(damageSpeed <= DeathSpeedLimit)
    {
        const auto dsSq = util::square(damageSpeed);
        m_lara.m_state.health -= core::LaraHealth * dsSq / DeathSpeedLimitSq;
    }
    else
    {
        m_lara.m_state.health = -1_hp;
    }
    return m_lara.m_state.health <= 0_hp;
}

loader::file::BoundingBox AbstractStateHandler::getBoundingBox() const
{
    return m_lara.getBoundingBox();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::addSwimToDiveKeypressDuration(const core::Frame n) noexcept
{
    m_lara.addSwimToDiveKeypressDuration(n);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setSwimToDiveKeypressDuration(const core::Frame n) noexcept
{
    m_lara.setSwimToDiveKeypressDuration(n);
}

// ReSharper disable once CppMemberFunctionMayBeConst
core::Frame AbstractStateHandler::getSwimToDiveKeypressDuration() const
{
    return m_lara.getSwimToDiveKeypressDuration();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setUnderwaterState(const UnderwaterState u) noexcept
{
    m_lara.setUnderwaterState(u);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setCameraRotationAroundCenter(const core::Angle x, const core::Angle y)
{
    m_lara.setCameraRotationAroundCenter(x, y);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setCameraRotationAroundCenterX(const core::Angle x)
{
    m_lara.setCameraRotationAroundCenterX(x);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setCameraRotationAroundCenterY(const core::Angle y)
{
    m_lara.setCameraRotationAroundCenterY(y);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setCameraEyeCenterDistance(const core::Length d)
{
    m_lara.setCameraEyeCenterDistance(d);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::setCameraModifier(const CameraModifier k)
{
    m_lara.setCameraModifier(k);
}

void AbstractStateHandler::jumpAgainstWall(CollisionInfo& collisionInfo)
{
    applyShift(collisionInfo);
    if(collisionInfo.collisionType == CollisionInfo::AxisColl::Left)
    {
        m_lara.m_state.rotation.Y += 5_deg;
    }
    else if(collisionInfo.collisionType == CollisionInfo::AxisColl::Right)
    {
        m_lara.m_state.rotation.Y -= 5_deg;
    }
    else if(collisionInfo.collisionType == CollisionInfo::AxisColl::Top)
    {
        if(m_lara.m_state.fallspeed <= 0_spd)
        {
            m_lara.m_state.fallspeed = 1_spd;
        }
    }
    else if(collisionInfo.collisionType == CollisionInfo::AxisColl::TopFront)
    {
        m_lara.m_state.position.position += util::pitch(core::DefaultCollisionRadius, m_lara.m_state.rotation.Y);
        m_lara.m_state.speed = 0_spd;
        collisionInfo.mid.floorSpace.y = 0_len;
        if(m_lara.m_state.fallspeed < 0_spd)
        {
            m_lara.m_state.fallspeed = 16_spd;
        }
    }
}

void AbstractStateHandler::checkJumpWallSmash(CollisionInfo& collisionInfo)
{
    applyShift(collisionInfo);

    if(collisionInfo.collisionType == CollisionInfo::AxisColl::None)
    {
        return;
    }

    if(collisionInfo.collisionType == CollisionInfo::AxisColl::Front
       || collisionInfo.collisionType == CollisionInfo::AxisColl::TopBottom)
    {
        setGoalAnimState(LaraStateId::FreeFall);
        m_lara.m_state.speed /= 4;
        setMovementAngle(getMovementAngle() - 180_deg);
        setAnimation(AnimationId::SMASH_JUMP, 481_frame);
        if(m_lara.m_state.fallspeed <= 0_spd)
        {
            m_lara.m_state.fallspeed = 1_spd;
        }
        return;
    }

    if(collisionInfo.collisionType == CollisionInfo::AxisColl::Left)
    {
        m_lara.m_state.rotation.Y += 5_deg;
        return;
    }
    if(collisionInfo.collisionType == CollisionInfo::AxisColl::Right)
    {
        m_lara.m_state.rotation.Y -= 5_deg;
        return;
    }

    if(collisionInfo.collisionType == CollisionInfo::AxisColl::TopFront)
    {
        m_lara.m_state.position.position += util::pitch(core::DefaultCollisionRadius, collisionInfo.facingAngle);
        m_lara.m_state.speed = 0_spd;
        collisionInfo.mid.floorSpace.y = 0_len;
        if(m_lara.m_state.fallspeed <= 0_spd)
        {
            m_lara.m_state.fallspeed = 16_spd;
        }
    }

    if(collisionInfo.collisionType == CollisionInfo::AxisColl::Top && m_lara.m_state.fallspeed <= 0_spd)
    {
        m_lara.m_state.fallspeed = 1_spd;
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AbstractStateHandler::laraUpdateImpl()
{
    m_lara.updateImpl();
}
} // namespace lara
} // namespace engine

#include "inventory.h"

#include "engine.h"
#include "items_tr1.h"
#include "laranode.h"

#include <boost/log/trivial.hpp>

namespace engine
{
void Inventory::put(const core::TypeId id, const size_t quantity)
{
    BOOST_LOG_TRIVIAL(debug) << "Item " << toString(id.get_as<TR1ItemId>()) << " added to inventory";

    switch(id.get_as<TR1ItemId>())
    {
    case TR1ItemId::PistolsSprite:
    case TR1ItemId::Pistols: m_inventory[TR1ItemId::Pistols] += quantity; break;
    case TR1ItemId::ShotgunSprite:
    case TR1ItemId::Shotgun:
        if(const auto clips = count(TR1ItemId::ShotgunAmmoSprite))
        {
            tryTake(TR1ItemId::ShotgunAmmoSprite, clips);
            m_engine.getLara().shotgunAmmo.ammo += 12 * clips;
        }
        m_engine.getLara().shotgunAmmo.ammo += 12 * quantity;
        // TODO replaceItems( ShotgunSprite, ShotgunAmmoSprite );
        m_inventory[TR1ItemId::Shotgun] = 1;
        break;
    case TR1ItemId::MagnumsSprite:
    case TR1ItemId::Magnums:
        if(const auto clips = count(TR1ItemId::MagnumAmmoSprite))
        {
            tryTake(TR1ItemId::MagnumAmmoSprite, clips);
            m_engine.getLara().revolverAmmo.ammo += 50 * clips;
        }
        m_engine.getLara().revolverAmmo.ammo += 50 * quantity;
        // TODO replaceItems( MagnumsSprite, MagnumAmmoSprite );
        m_inventory[TR1ItemId::Magnums] = 1;
        break;
    case TR1ItemId::UzisSprite:
    case TR1ItemId::Uzis:
        if(const auto clips = count(TR1ItemId::UziAmmoSprite))
        {
            tryTake(TR1ItemId::UziAmmoSprite, clips);
            m_engine.getLara().uziAmmo.ammo += 100 * clips;
        }
        m_engine.getLara().uziAmmo.ammo += 100 * quantity;
        // TODO replaceItems( UzisSprite, UziAmmoSprite );
        m_inventory[TR1ItemId::Uzis] = 1;
        break;
    case TR1ItemId::ShotgunAmmoSprite:
    case TR1ItemId::ShotgunAmmo:
        if(count(TR1ItemId::ShotgunSprite) > 0)
            m_engine.getLara().shotgunAmmo.ammo += 12;
        else
            m_inventory[TR1ItemId::ShotgunAmmo] += quantity;
        break;
    case TR1ItemId::MagnumAmmoSprite:
    case TR1ItemId::MagnumAmmo:
        if(count(TR1ItemId::MagnumsSprite) > 0)
            m_engine.getLara().revolverAmmo.ammo += 50;
        else
            m_inventory[TR1ItemId::MagnumAmmo] += quantity;
        break;
    case TR1ItemId::UziAmmoSprite:
    case TR1ItemId::UziAmmo:
        if(count(TR1ItemId::UzisSprite) > 0)
            m_engine.getLara().uziAmmo.ammo += 100;
        else
            m_inventory[TR1ItemId::UziAmmo] += quantity;
        break;
    case TR1ItemId::SmallMedipackSprite:
    case TR1ItemId::SmallMedipack: m_inventory[TR1ItemId::SmallMedipack] += quantity; break;
    case TR1ItemId::LargeMedipackSprite:
    case TR1ItemId::LargeMedipack: m_inventory[TR1ItemId::LargeMedipack] += quantity; break;
    case TR1ItemId::Puzzle1Sprite:
    case TR1ItemId::Puzzle1: m_inventory[TR1ItemId::Puzzle1] += quantity; break;
    case TR1ItemId::Puzzle2Sprite:
    case TR1ItemId::Puzzle2: m_inventory[TR1ItemId::Puzzle2] += quantity; break;
    case TR1ItemId::Puzzle3Sprite:
    case TR1ItemId::Puzzle3: m_inventory[TR1ItemId::Puzzle3] += quantity; break;
    case TR1ItemId::Puzzle4Sprite:
    case TR1ItemId::Puzzle4: m_inventory[TR1ItemId::Puzzle4] += quantity; break;
    case TR1ItemId::LeadBarSprite:
    case TR1ItemId::LeadBar: m_inventory[TR1ItemId::LeadBar] += quantity; break;
    case TR1ItemId::Key1Sprite:
    case TR1ItemId::Key1: m_inventory[TR1ItemId::Key1] += quantity; break;
    case TR1ItemId::Key2Sprite:
    case TR1ItemId::Key2: m_inventory[TR1ItemId::Key2] += quantity; break;
    case TR1ItemId::Key3Sprite:
    case TR1ItemId::Key3: m_inventory[TR1ItemId::Key3] += quantity; break;
    case TR1ItemId::Key4Sprite:
    case TR1ItemId::Key4: m_inventory[TR1ItemId::Key4] += quantity; break;
    case TR1ItemId::Item141:
    case TR1ItemId::Item148: m_inventory[TR1ItemId::Item148] += quantity; break;
    case TR1ItemId::Item142:
    case TR1ItemId::Item149: m_inventory[TR1ItemId::Item149] += quantity; break;
    case TR1ItemId::ScionPiece1:
    case TR1ItemId::ScionPiece2:
    case TR1ItemId::ScionPiece5: m_inventory[TR1ItemId::ScionPiece5] += quantity; break;
    default:
        BOOST_LOG_TRIVIAL(warning) << "Cannot add item " << toString(id.get_as<TR1ItemId>()) << " to inventory";
        return;
    }
}

bool Inventory::tryUse(const TR1ItemId id)
{
    if(id == TR1ItemId::Shotgun || id == TR1ItemId::ShotgunSprite)
    {
        if(count(TR1ItemId::Shotgun) == 0)
            return false;

        m_engine.getLara().requestedGunType = LaraNode::WeaponId::Shotgun;
        if(m_engine.getLara().getHandStatus() == HandStatus::None
           && m_engine.getLara().gunType == m_engine.getLara().requestedGunType)
        {
            m_engine.getLara().gunType = LaraNode::WeaponId::None;
        }
    }
    else if(id == TR1ItemId::Pistols || id == TR1ItemId::PistolsSprite)
    {
        if(count(TR1ItemId::Pistols) == 0)
            return false;

        m_engine.getLara().requestedGunType = LaraNode::WeaponId::Pistols;
        if(m_engine.getLara().getHandStatus() == HandStatus::None
           && m_engine.getLara().gunType == m_engine.getLara().requestedGunType)
        {
            m_engine.getLara().gunType = LaraNode::WeaponId::None;
        }
    }
    else if(id == TR1ItemId::Magnums || id == TR1ItemId::MagnumsSprite)
    {
        if(count(TR1ItemId::Magnums) == 0)
            return false;

        m_engine.getLara().requestedGunType = LaraNode::WeaponId::AutoPistols;
        if(m_engine.getLara().getHandStatus() == HandStatus::None
           && m_engine.getLara().gunType == m_engine.getLara().requestedGunType)
        {
            m_engine.getLara().gunType = LaraNode::WeaponId::None;
        }
    }
    else if(id == TR1ItemId::Uzis || id == TR1ItemId::UzisSprite)
    {
        if(count(TR1ItemId::Uzis) == 0)
            return false;

        m_engine.getLara().requestedGunType = LaraNode::WeaponId::Uzi;
        if(m_engine.getLara().getHandStatus() == HandStatus::None
           && m_engine.getLara().gunType == m_engine.getLara().requestedGunType)
        {
            m_engine.getLara().gunType = LaraNode::WeaponId::None;
        }
    }
    else if(id == TR1ItemId::LargeMedipack || id == TR1ItemId::LargeMedipackSprite)
    {
        if(count(TR1ItemId::LargeMedipack) == 0)
            return false;

        if(m_engine.getLara().m_state.health <= 0_hp || m_engine.getLara().m_state.health >= core::LaraHealth)
        {
            return false;
        }

        m_engine.getLara().m_state.health += 1000_hp;
        if(m_engine.getLara().m_state.health > core::LaraHealth)
        {
            m_engine.getLara().m_state.health = core::LaraHealth;
        }
        tryTake(TR1ItemId::LargeMedipackSprite);
        m_engine.getAudioEngine().playSound(TR1SoundId::LaraSigh, &m_engine.getLara().m_state);
    }
    else if(id == TR1ItemId::SmallMedipack || id == TR1ItemId::SmallMedipackSprite)
    {
        if(count(TR1ItemId::SmallMedipack) == 0)
            return false;

        if(m_engine.getLara().m_state.health <= 0_hp || m_engine.getLara().m_state.health >= core::LaraHealth)
        {
            return false;
        }

        m_engine.getLara().m_state.health += 500_hp;
        if(m_engine.getLara().m_state.health > core::LaraHealth)
        {
            m_engine.getLara().m_state.health = core::LaraHealth;
        }
        tryTake(TR1ItemId::SmallMedipackSprite);
        m_engine.getAudioEngine().playSound(TR1SoundId::LaraSigh, &m_engine.getLara().m_state);
    }

    return true;
}

bool Inventory::tryTake(const TR1ItemId id, const size_t quantity)
{
    BOOST_LOG_TRIVIAL(debug) << "Taking item " << toString(id) << " from inventory";

    const auto it = m_inventory.find(id);
    if(it == m_inventory.end())
        return false;

    if(it->second < quantity)
        return false;

    if(it->second == quantity)
        m_inventory.erase(it);
    else
        m_inventory[id] -= quantity;

    return true;
}
}
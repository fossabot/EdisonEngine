/*
 * Copyright 2002 - Florian Schulze <crow@icculus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * This file is part of vt.
 *
 */

#include "engine/animationcontroller.h"
#include "engine/inputhandler.h"
#include "engine/laracontroller.h"
#include "level.h"
#include "render/lightselector.h"
#include "render/textureanimator.h"
#include "tr1level.h"
#include "tr2level.h"
#include "tr3level.h"
#include "tr4level.h"
#include "tr5level.h"
#include "util/vmath.h"

#include <boost/lexical_cast.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <algorithm>
#include <stack>
#include <set>
#include <EffectHandler.h>

using namespace level;

namespace
{
    const irr::video::SColor WaterColor{0, 149, 229, 229};
}

Level::~Level() = default;

/// \brief reads the mesh data.
void Level::readMeshData(loader::io::SDLReader& reader)
{
    uint32_t meshDataWords = reader.readU32();
    const auto basePos = reader.tell();

    const auto meshDataSize = meshDataWords * 2;
    reader.skip(meshDataSize);

    reader.readVector(m_meshIndices, reader.readU32());
    const auto endPos = reader.tell();

    m_meshes.clear();

    size_t meshDataPos = 0;
    for( size_t i = 0; i < m_meshIndices.size(); i++ )
    {
        std::replace(m_meshIndices.begin(), m_meshIndices.end(), meshDataPos, i);

        reader.seek(basePos + std::streamoff(meshDataPos));

        if( gameToEngine(m_gameVersion) >= Engine::TR4 )
            m_meshes.emplace_back(*loader::Mesh::readTr4(reader));
        else
            m_meshes.emplace_back(*loader::Mesh::readTr1(reader));

        for( size_t j = 0; j < m_meshIndices.size(); j++ )
        {
            if( m_meshIndices[j] > meshDataPos )
            {
                meshDataPos = m_meshIndices[j];
                break;
            }
        }
    }

    reader.seek(endPos);
}

/// \brief reads frame and moveable data.
void Level::readPoseDataAndModels(loader::io::SDLReader& reader)
{
    m_poseData.resize(reader.readU32());
    reader.readVector(m_poseData, m_poseData.size());

    m_animatedModels.resize(reader.readU32());
    for( std::unique_ptr<loader::AnimatedModel>& model : m_animatedModels )
    {
        if( gameToEngine(m_gameVersion) < Engine::TR5 )
        {
            model = loader::AnimatedModel::readTr1(reader);
            // Disable unused skybox polygons.
            if( gameToEngine(m_gameVersion) == Engine::TR3 && model->type == 355 )
            {
                m_meshes[m_meshIndices[model->firstMesh]].colored_triangles.resize(16);
            }
        }
        else
        {
            model = loader::AnimatedModel::readTr5(reader);
        }
    }
}

std::unique_ptr<Level> Level::createLoader(const std::string& filename, Game game_version)
{
    size_t len2 = 0;

    for( size_t i = 0; i < filename.length(); i++ )
    {
        if( filename[i] == '/' || filename[i] == '\\' )
        {
            len2 = i;
        }
    }

    std::string sfxPath;

    if( len2 > 0 )
    {
        sfxPath = filename.substr(0, len2 + 1) + "MAIN.SFX";
    }
    else
    {
        sfxPath = "MAIN.SFX";
    }

    loader::io::SDLReader reader(filename);
    if( !reader.isOpen() )
        return nullptr;

    if( game_version == Game::Unknown )
        game_version = probeVersion(reader, filename);
    if( game_version == Game::Unknown )
        return nullptr;

    reader.seek(0);
    return createLoader(std::move(reader), game_version, sfxPath);
}

/** \brief reads the level.
  *
  * Takes a SDL_RWop and the game_version of the file and reads the structures into the members of TR_Level.
  */
std::unique_ptr<Level> Level::createLoader(loader::io::SDLReader&& reader, Game game_version, const std::string& sfxPath)
{
    if( !reader.isOpen() )
        return nullptr;

    std::unique_ptr<Level> result;

    switch( game_version )
    {
    case Game::TR1:
        result.reset(new TR1Level(game_version, std::move(reader)));
        break;
    case Game::TR1Demo:
    case Game::TR1UnfinishedBusiness:
        result.reset(new TR1Level(game_version, std::move(reader)));
        result->m_demoOrUb = true;
        break;
    case Game::TR2:
        result.reset(new TR2Level(game_version, std::move(reader)));
        break;
    case Game::TR2Demo:
        result.reset(new TR2Level(game_version, std::move(reader)));
        result->m_demoOrUb = true;
        break;
    case Game::TR3:
        result.reset(new TR3Level(game_version, std::move(reader)));
        break;
    case Game::TR4:
    case Game::TR4Demo:
        result.reset(new TR4Level(game_version, std::move(reader)));
        break;
    case Game::TR5:
        result.reset(new TR5Level(game_version, std::move(reader)));
        break;
    default:
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid game version"));
    }

    result->m_sfxPath = sfxPath;
    return result;
}

Game Level::probeVersion(loader::io::SDLReader& reader, const std::string& filename)
{
    if( !reader.isOpen() || filename.length() < 5 )
        return Game::Unknown;

    std::string ext;
    ext += filename[filename.length() - 4];
    ext += toupper(filename[filename.length() - 3]);
    ext += toupper(filename[filename.length() - 2]);
    ext += toupper(filename[filename.length() - 1]);

    reader.seek(0);
    uint8_t check[4];
    reader.readBytes(check, 4);

    Game ret = Game::Unknown;
    if( ext == ".PHD" )
    {
        if( check[0] == 0x20 && check[1] == 0x00 && check[2] == 0x00 && check[3] == 0x00 )
        {
            ret = Game::TR1;
        }
    }
    else if( ext == ".TUB" )
    {
        if( check[0] == 0x20 && check[1] == 0x00 && check[2] == 0x00 && check[3] == 0x00 )
        {
            ret = Game::TR1UnfinishedBusiness;
        }
    }
    else if( ext == ".TR2" )
    {
        if( check[0] == 0x2D && check[1] == 0x00 && check[2] == 0x00 && check[3] == 0x00 )
        {
            ret = Game::TR2;
        }
        else if( (check[0] == 0x38 || check[0] == 0x34) && check[1] == 0x00 && (check[2] == 0x18 || check[2] == 0x08) && check[3] == 0xFF )
        {
            ret = Game::TR3;
        }
    }
    else if( ext == ".TR4" )
    {
        if( check[0] == 0x54 && // T
            check[1] == 0x52 && // R
            check[2] == 0x34 && // 4
            check[3] == 0x00 )
        {
            ret = Game::TR4;
        }
        else if( check[0] == 0x54 && // T
            check[1] == 0x52 && // R
            check[2] == 0x34 && // 4
            check[3] == 0x63 ) //
        {
            ret = Game::TR4;
        }
        else if( check[0] == 0xF0 && // T
            check[1] == 0xFF && // R
            check[2] == 0xFF && // 4
            check[3] == 0xFF )
        {
            ret = Game::TR4;
        }
    }
    else if( ext == ".TRC" )
    {
        if( check[0] == 0x54 && // T
            check[1] == 0x52 && // R
            check[2] == 0x34 && // C
            check[3] == 0x00 )
        {
            ret = Game::TR5;
        }
    }

    return ret;
}

loader::StaticMesh* Level::findStaticMeshById(uint32_t meshId)
{
    for( size_t i = 0; i < m_staticMeshes.size(); i++ )
        if( m_staticMeshes[i].id == meshId )
            return &m_staticMeshes[i];

    return nullptr;
}

const loader::StaticMesh* Level::findStaticMeshById(uint32_t meshId) const
{
    for( size_t i = 0; i < m_staticMeshes.size(); i++ )
        if( m_staticMeshes[i].id == meshId )
            return &m_staticMeshes[i];

    return nullptr;
}

int Level::findStaticMeshIndexById(uint32_t meshId) const
{
    for( size_t i = 0; i < m_staticMeshes.size(); i++ )
    {
        if( m_staticMeshes[i].id == meshId )
        {
            BOOST_ASSERT(m_staticMeshes[i].mesh < m_meshIndices.size());
            return m_meshIndices[m_staticMeshes[i].mesh];
        }
    }

    return -1;
}

boost::optional<size_t> Level::findAnimatedModelIndexForType(uint32_t type) const
{
    for( size_t i = 0; i < m_animatedModels.size(); i++ )
        if( m_animatedModels[i]->type == type )
            return i;

    return boost::none;
}

boost::optional<size_t> Level::findSpriteSequenceForType(uint32_t type) const
{
    for( size_t i = 0; i < m_spriteSequences.size(); i++ )
        if( m_spriteSequences[i].type == type )
            return i;

    return boost::none;
}

loader::Item* Level::findItemByType(int32_t type)
{
    for( size_t i = 0; i < m_items.size(); i++ )
        if( m_items[i].type == type )
            return &m_items[i];

    return nullptr;
}

loader::AnimatedModel* Level::findModelByType(uint32_t type)
{
    for( size_t i = 0; i < m_animatedModels.size(); i++ )
        if( m_animatedModels[i]->type == type )
            return m_animatedModels[i].get();

    return nullptr;
}

std::vector<irr::video::ITexture*> Level::createTextures(irr::scene::ISceneManager* mgr)
{
    BOOST_ASSERT(!m_textures.empty());
    std::vector<irr::video::ITexture*> textures;
    for( size_t i = 0; i < m_textures.size(); ++i )
    {
        loader::DWordTexture& texture = m_textures[i];
        textures.emplace_back(texture.toTexture(mgr, i));
    }
    return textures;
}

std::map<loader::TextureLayoutProxy::TextureKey, irr::video::SMaterial> Level::createMaterials(const std::vector<irr::video::ITexture*>& textures)
{
    const auto texMask = gameToEngine(m_gameVersion) == Engine::TR4 ? loader::TextureIndexMaskTr4 : loader::TextureIndexMask;
    std::map<loader::TextureLayoutProxy::TextureKey, irr::video::SMaterial> materials;
    for( loader::TextureLayoutProxy& proxy : m_textureProxies )
    {
        const auto& key = proxy.textureKey;
        if( materials.find(key) != materials.end() )
            continue;

        materials[key] = loader::TextureLayoutProxy::createMaterial(textures[key.tileAndFlag & texMask], key.blendingMode);
    }
    return materials;
}

#ifndef NDEBUG
using StateMap = std::map<loader::LaraStateId, std::map<loader::AnimationId, std::map<loader::AnimationId, std::string>>>;

void loadAnim(StateMap& map, loader::AnimationId aid, uint16_t ofs, const loader::Animation& anim, const Level* level)
{
    map[static_cast<loader::LaraStateId>(anim.state_id)][aid][static_cast<loader::AnimationId>(anim.nextAnimation - ofs)] = "@";
    for( size_t i = 0; i < anim.transitionsCount; ++i )
    {
        auto tIdx = anim.transitionsIndex + i;
        BOOST_ASSERT(tIdx < level->m_transitions.size());
        const loader::Transitions& tr = level->m_transitions[tIdx];

        for( auto j = tr.firstTransitionCase; j < tr.firstTransitionCase + tr.transitionCaseCount; ++j )
        {
            BOOST_ASSERT(j < level->m_transitionCases.size());
            const loader::TransitionCase& trc = level->m_transitionCases[j];

            loader::AnimationId a = static_cast<loader::AnimationId>(trc.targetAnimation - ofs);

            map[static_cast<loader::LaraStateId>(anim.state_id)][aid][a] = {};
        }
    }
}

void dumpAnims(const loader::AnimatedModel& model, const Level* level)
{
    StateMap map;
    for( const auto& fm : model.frameMapping )
    {
        loadAnim(map, static_cast<loader::AnimationId>(fm.first - model.animationIndex), model.animationIndex, level->m_animations[fm.first], level);
    }

    std::cerr << "@startuml\n\n";
    std::cerr << "[*] --> Stop\n\n";

    for( const auto& state : map )
    {
        const char* st = toString(state.first);
        if( st == nullptr )
        {
            st = "???";
        }
        std::cerr << "state \"" << st << "\" as s" << static_cast<uint16_t>(state.first) << " { \n";

        for( const auto& anim : state.second )
        {
            const char* an = toString(anim.first);
            if( an == nullptr )
            {
                an = "???";
            }
            std::cerr << "    state \"" << an << "\" as a" << static_cast<uint16_t>(anim.first) << "\n";
        }

        std::cerr << "}\n\n";
    }

    for( const auto& state : map )
    {
        for( const auto& anim : state.second )
        {
            for( const auto& targ : anim.second )
            {
                if( targ.second.empty() )
                    std::cerr << "a" << static_cast<uint16_t>(anim.first) << " --> a" << static_cast<uint16_t>(targ.first) << "\n";
                else
                    std::cerr << "a" << static_cast<uint16_t>(anim.first) << " --> a" << static_cast<uint16_t>(targ.first) << " : " << targ.second << "\n";
            }
        }
    }

    std::cerr << "@enduml\n";
}

#endif

engine::LaraController* Level::createItems(irr::scene::ISceneManager* mgr, const std::vector<irr::scene::ISkinnedMesh*>& skinnedMeshes, const std::vector<irr::video::ITexture*>& textures)
{
    engine::LaraController* lara = nullptr;
    int id = -1;
    for( loader::Item& item : m_items )
    {
        ++id;

        BOOST_ASSERT(item.room < m_rooms.size());
        loader::Room& room = m_rooms[item.room];

        if(const auto meshIdx = findAnimatedModelIndexForType(item.type))
        {
            //BOOST_ASSERT(!findSpriteSequenceForType(item.type));
            BOOST_ASSERT(*meshIdx < skinnedMeshes.size());
            irr::scene::IAnimatedMeshSceneNode* node;

            if( item.type == 0 )
            {
                node = mgr->addAnimatedMeshSceneNode(skinnedMeshes[*meshIdx], nullptr); // Lara doesn't have a scene graph owner
            }
            else
            {
                node = mgr->addAnimatedMeshSceneNode(skinnedMeshes[*meshIdx], room.node);
            }

            std::string name = "item";
            name += std::to_string(id);
            name += "(type";
            name += std::to_string(item.type);
            name += "/animatedModel)";
            node->setName(name.c_str());

            //node->setAutomaticCulling(false);
            //node->setDebugDataVisible(irr::scene::EDS_SKELETON|irr::scene::EDS_BBOX_ALL|irr::scene::EDS_MESH_WIRE_OVERLAY);
            //node->setDebugDataVisible(irr::scene::EDS_FULL);
            node->setAnimationSpeed(30);
            node->setLoopMode(false);
            auto animationController = std::make_shared<engine::MeshAnimationController>(this, *m_animatedModels[*meshIdx], node, name + ":animator");

            if( item.type == 0 )
            {
                lara = new engine::LaraController(this, animationController, node, name + ":controller", &room, &item);
                m_itemControllers[id].reset( lara );
#ifndef NDEBUG
                dumpAnims(*m_animatedModels[*meshIdx], this);
#endif
            }
            else if(item.type == 35)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_35_CollapsibleFloor>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type == 36)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_SwingingBlade>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type == 41)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_41_TrapDoorUp>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type >= 48 && item.type <= 51)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_Block>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type == 52)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_TallBlock>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type == 55)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_55_Switch>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type >= 57 && item.type <= 64)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_Door>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type >= 65 && item.type <= 66)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_TrapDoorDown>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type == 68)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_68_BridgeFlat>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type == 69)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_69_BridgeSlope1>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else if(item.type == 70)
            {
                m_itemControllers[id] = std::make_unique<engine::ItemController_70_BridgeSlope2>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }
            else
            {
                m_itemControllers[id] = std::make_unique<engine::DummyItemController>(this, animationController, node, name + ":controller", &room, &item);
                animationController->playLocalAnimation(0);
            }

            node->addShadowVolumeSceneNode();

            m_fx->addShadowToNode(m_itemControllers[id]->getSceneNode());

            for( irr::u32 i = 0; i < node->getMaterialCount(); ++i )
            {
                node->getMaterial(i).DiffuseColor = room.lightColor.toSColor(1 - room.darkness / 8191.0f);
                node->getMaterial(i).SpecularColor = node->getMaterial(i).DiffuseColor;
                node->getMaterial(i).EmissiveColor.set(0);
            }

            continue;
        }

        if(const auto sequenceId = findSpriteSequenceForType(item.type))
        {
            BOOST_ASSERT(!findAnimatedModelIndexForType(item.type));
            BOOST_ASSERT(*sequenceId < m_spriteSequences.size());
            const loader::SpriteSequence& spriteSequence = m_spriteSequences[*sequenceId];

            BOOST_ASSERT(spriteSequence.offset < m_spriteTextures.size());

            const loader::SpriteTexture& tex = m_spriteTextures[spriteSequence.offset];

            irr::core::vector2df dim{ static_cast<irr::f32>(tex.right_side - tex.left_side + 1), static_cast<irr::f32>(tex.bottom_side - tex.top_side + 1) };
            BOOST_ASSERT(dim.X > 0);
            BOOST_ASSERT(dim.Y > 0);

            irr::scene::IBillboardSceneNode* node = mgr->addBillboardSceneNode(room.node, dim, { 0,0,0 }, -1, 0, 0);
            node->setMaterialType(irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);
            node->getMaterial(0).BlendOperation = irr::video::EBO_ADD;
            node->getMaterial(0).EmissiveColor.set(0);
            node->setMaterialFlag(irr::video::EMF_LIGHTING, true);
            node->setMaterialTexture(0, textures[tex.texture]);
            node->getMaterial(0).setTextureMatrix(0, tex.buildTextureMatrix());

            std::string name = "item";
            name += std::to_string(id);
            name += "(type";
            name += std::to_string(item.type);
            name += "/spriteSequence)";
            node->setName(name.c_str());

            m_itemControllers[id] = std::make_unique<engine::DummyItemController>(this, nullptr, node, name + ":controller", &room, &item);
            m_itemControllers[id]->setYRotation(core::Angle{ item.rotation });
            m_itemControllers[id]->setPosition(core::ExactTRCoordinates(item.position - core::TRCoordinates(0, tex.bottom_side, 0)));

            continue;
        }

        BOOST_LOG_TRIVIAL(error) << "No static mesh or animated model for item " << id << "/type " << int(item.type);
    }

    return lara;
}

void Level::loadAnimFrame(irr::u32 frameIdx, irr::u32 frameOffset, const loader::AnimatedModel& model, const loader::Animation& animation, irr::scene::ISkinnedMesh* skinnedMesh, gsl::not_null<const int16_t*>& pData, irr::core::aabbox3di& bbox)
{
    uint16_t angleSetOfs = 10;

    for( size_t meshIdx = 0; meshIdx < model.meshCount; meshIdx++ )
    {
        auto joint = skinnedMesh->getAllJoints()[meshIdx];
        auto pKey = skinnedMesh->addPositionKey(joint);
        pKey->frame = frameIdx + frameOffset;

        if( meshIdx == 0 )
        {
            bbox.MinEdge = {pData[0], pData[2], pData[4]};
            bbox.MaxEdge = {pData[1], pData[3], pData[5]};
            pKey->position.set(pData[6], static_cast<irr::f32>(-pData[7]), pData[8]);
        }
        else
        {
            BOOST_ASSERT(model.boneTreeIndex + 4 * meshIdx <= m_boneTrees.size());
            const int32_t* boneTreeData = &m_boneTrees[model.boneTreeIndex + (meshIdx - 1) * 4];
            pKey->position.set(static_cast<irr::f32>(boneTreeData[1]), static_cast<irr::f32>(-boneTreeData[2]), static_cast<irr::f32>(boneTreeData[3]));
        }

        auto rKey = skinnedMesh->addRotationKey(joint);
        rKey->frame = frameIdx + frameOffset;

        auto temp2 = pData[angleSetOfs++];
        auto temp1 = pData[angleSetOfs++];

        irr::core::vector3df rot;
        rot.X = static_cast<irr::f32>((temp1 & 0x3ff0) >> 4);
        rot.Y = -static_cast<irr::f32>(((temp1 & 0x000f) << 6) | ((temp2 & 0xfc00) >> 10));
        rot.Z = static_cast<irr::f32>(temp2 & 0x03ff);
        rot *= 360 / 1024.0;

        rKey->rotation = util::trRotationToQuat(rot);
    }

    pData = pData.get() + animation.poseDataSize;
}

loader::AnimatedModel::FrameRange Level::loadAnimation(irr::u32& frameOffset, const loader::AnimatedModel& model, const loader::Animation& animation, irr::scene::ISkinnedMesh* skinnedMesh)
{
    BOOST_ASSERT(animation.poseDataOffset % 2 == 0);
    gsl::not_null<const int16_t*> pData = &m_poseData[animation.poseDataOffset / 2];
    const int16_t* lastPData = nullptr;

    irr::core::aabbox3di bbox;
    // prepend the first frame
    loadAnimFrame(0, frameOffset, model, animation, skinnedMesh, pData, bbox);
    frameOffset += animation.stretchFactor;

    const auto firstLinearFrame = frameOffset;

    std::map<irr::u32, irr::core::aabbox3di> bboxes;
    pData = &m_poseData[animation.poseDataOffset / 2];
    for( irr::u32 i = 0; i <= gsl::narrow<irr::u32>(animation.lastFrame - animation.firstFrame); i += animation.stretchFactor )
    {
        lastPData = pData;
        loadAnimFrame(0, frameOffset, model, animation, skinnedMesh, pData, bbox);
        bboxes.insert(std::make_pair(i, bbox));
        frameOffset += animation.stretchFactor;
    }

    irr::u32 framePatch = 0;
    // is necessary, create pseudo-frames, because otherwise irrlicht thinks
    // there's no animation at all
    while( animation.firstFrame >= animation.lastFrame + framePatch )
    {
        pData = lastPData;
        loadAnimFrame(0, frameOffset, model, animation, skinnedMesh, pData, bbox);
        frameOffset += animation.stretchFactor;
        ++framePatch;
    }
    BOOST_LOG_TRIVIAL(debug) << "Framepatch: " << animation.firstFrame << " / " << animation.lastFrame << " + " << framePatch;

    // append the last frame again
    pData = lastPData;
    loadAnimFrame(0, frameOffset, model, animation, skinnedMesh, pData, bbox);
    frameOffset += animation.stretchFactor;

    return loader::AnimatedModel::FrameRange{firstLinearFrame, animation.firstFrame, animation.lastFrame + framePatch, std::move(bboxes)};
}

std::vector<irr::scene::ISkinnedMesh*> Level::createSkinnedMeshes(irr::scene::ISceneManager* mgr, const std::vector<irr::scene::SMesh*>& staticMeshes)
{
    BOOST_ASSERT(!m_animatedModels.empty());

    std::set<uint16_t> animStarts;
    for( const std::unique_ptr<loader::AnimatedModel>& model : m_animatedModels )
    {
        if( model->animationIndex == 0xffff )
        {
            BOOST_LOG_TRIVIAL(warning) << "Model 0x" << std::hex << reinterpret_cast<uintptr_t>(model.get()) << std::dec << " has animationIndex==0xffff";
            continue;
        }

        BOOST_ASSERT(animStarts.find(model->animationIndex) == animStarts.end());
        animStarts.insert(model->animationIndex);
    }
    animStarts.insert(gsl::narrow<uint16_t>(m_animations.size()));

    std::vector<irr::scene::ISkinnedMesh*> skinnedMeshes;

    for( const std::unique_ptr<loader::AnimatedModel>& model : m_animatedModels )
    {
        Expects(model != nullptr);
        irr::scene::ISkinnedMesh* skinnedMesh = mgr->createSkinnedMesh();
        skinnedMeshes.emplace_back(skinnedMesh);

        std::stack<irr::scene::ISkinnedMesh::SJoint*> parentStack;

        for( size_t modelMeshIdx = 0; modelMeshIdx < model->meshCount; ++modelMeshIdx )
        {
            BOOST_ASSERT(model->firstMesh + modelMeshIdx < m_meshIndices.size());
            const auto meshIndex = m_meshIndices[model->firstMesh + modelMeshIdx];
            BOOST_ASSERT(meshIndex < staticMeshes.size());
            const auto staticMesh = staticMeshes[meshIndex];
            irr::scene::ISkinnedMesh::SJoint* joint = skinnedMesh->addJoint();
            if( model->type == 0 )
            {
                if( modelMeshIdx == 7 )
                    joint->Name = "chest";
                else if( modelMeshIdx == 0 )
                    joint->Name = "hips";
            }

            // clone static mesh buffers to skinned mesh buffers
            for( irr::u32 meshBufIdx = 0; meshBufIdx < staticMesh->MeshBuffers.size(); ++meshBufIdx )
            {
                gsl::not_null<irr::scene::SSkinMeshBuffer*> skinMeshBuffer = skinnedMesh->addMeshBuffer();
                Expects(staticMesh->MeshBuffers[meshBufIdx]->getIndexType() == skinMeshBuffer->getIndexType());
                Expects(staticMesh->MeshBuffers[meshBufIdx]->getVertexType() == skinMeshBuffer->getVertexType());
                for( irr::u32 i = 0; i < staticMesh->MeshBuffers[meshBufIdx]->getIndexCount(); ++i )
                    skinMeshBuffer->Indices.push_back(staticMesh->MeshBuffers[meshBufIdx]->getIndices()[i]);

                for( irr::u32 i = 0; i < staticMesh->MeshBuffers[meshBufIdx]->getVertexCount(); ++i )
                {
                    skinMeshBuffer->Vertices_Standard.push_back(static_cast<irr::video::S3DVertex*>(staticMesh->MeshBuffers[meshBufIdx]->getVertices())[i]);

                    auto w = skinnedMesh->addWeight(joint);
                    w->buffer_id = skinnedMesh->getMeshBuffers().size() - 1;
                    w->strength = 1.0f;
                    w->vertex_id = i;
                }

                skinMeshBuffer->Material = staticMesh->MeshBuffers[meshBufIdx]->getMaterial();

                skinMeshBuffer->setDirty();
                skinMeshBuffer->boundingBoxNeedsRecalculated();
                skinMeshBuffer->recalculateBoundingBox();
            }

            if( modelMeshIdx == 0 )
            {
                parentStack.push(joint);
                continue;
            }

            auto pred = skinnedMesh->getAllJoints()[modelMeshIdx - 1];

            irr::scene::ISkinnedMesh::SJoint* parent = nullptr;
            BOOST_ASSERT(model->boneTreeIndex + 4 * modelMeshIdx <= m_boneTrees.size());
            const int32_t* boneTreeData = &m_boneTrees[model->boneTreeIndex + (modelMeshIdx - 1) * 4];

            switch( boneTreeData[0] )
            {
            case 0: // use predecessor
                parent = pred;
                parent->Children.push_back(joint);
                break;
            case 2: // push
                parent = pred;
                parent->Children.push_back(joint);
                parentStack.push(parent);
                break;
            case 1: // pop
                if( parentStack.empty() )
                    throw std::runtime_error("Invalid skeleton stack operation: cannot pop from empty stack");
                parent = parentStack.top();
                parent->Children.push_back(joint);
                parentStack.pop();
                break;
            case 3: // top
                if( parentStack.empty() )
                    throw std::runtime_error("Invalid skeleton stack operation: cannot take top of empty stack");
                parent = parentStack.top();
                parent->Children.push_back(joint);
                break;
            default:
                throw std::runtime_error("Invalid skeleton stack operation");
            }
        }

        const auto currentAnimIt = animStarts.find(model->animationIndex);

        if( currentAnimIt == animStarts.end() )
            continue;

        irr::u32 currentAnimOffset = 0;
        const auto nextAnimIdx = *std::next(currentAnimIt);

        for( auto currentAnimIdx = model->animationIndex; currentAnimIdx < nextAnimIdx; ++currentAnimIdx )
        {
            if( currentAnimIdx >= m_animations.size() )
                continue;

            const loader::Animation& animation = m_animations[currentAnimIdx];
            model->frameMapping.emplace(std::make_pair(currentAnimIdx, loadAnimation(currentAnimOffset, *model, animation, skinnedMesh)));
        }

#ifndef NDEBUG
        BOOST_LOG_TRIVIAL(debug) << "Model 0x" << std::hex << reinterpret_cast<uintptr_t>(model.get()) << std::dec << " frame mapping:";
        for( const auto& fm : model->frameMapping )
        {
            BOOST_LOG_TRIVIAL(debug) << "  - anim " << fm.first << ": offset=" << fm.second.offset << ", first=" << fm.second.firstFrame << ", last=" << fm.second.lastFrame;
        }
#endif

        skinnedMesh->finalize();
    }

    return skinnedMeshes;
}

irr::video::ITexture* Level::createSolidColorTex(irr::scene::ISceneManager* mgr, uint8_t color) const
{
    irr::video::SColor pixels[2][2];
    pixels[0][0].set(m_palette->color[color].a, m_palette->color[color].r, m_palette->color[color].g, m_palette->color[color].b);
    pixels[1][0] = pixels[0][0];
    pixels[0][1] = pixels[0][0];
    pixels[1][1] = pixels[0][0];

    auto img = mgr->getVideoDriver()->createImageFromData(irr::video::ECF_A8R8G8B8, {2, 2}, &pixels[0][0]);
    irr::io::path p;
    p = "tex_color";
    p += boost::lexical_cast<std::string>(int(color)).c_str();
    p += ".png";
    auto tex = mgr->getVideoDriver()->addTexture(p, img);

    mgr->getFileSystem()->changeWorkingDirectoryTo("dump");
    mgr->getVideoDriver()->writeImageToFile(img, p);
    mgr->getFileSystem()->changeWorkingDirectoryTo("..");

    img->drop();
    return tex;
}

void Level::toIrrlicht(irr::IrrlichtDevice* device)
{
    device->getSceneManager()->getVideoDriver()->setFog(WaterColor, irr::video::EFT_FOG_LINEAR, 1024, 1024 * 20, .003f, true, false);
    device->getSceneManager()->getVideoDriver()->setTextureCreationFlag(irr::video::ETCF_CREATE_MIP_MAPS, false);
    device->getSceneManager()->setLightManager(new render::LightSelector(*this, device->getSceneManager()));
    m_inputHandler = std::make_unique<engine::InputHandler>(device->getCursorControl());
    device->setEventReceiver(m_inputHandler.get());

    m_fx = std::make_shared<EffectHandler>(device, device->getVideoDriver()->getScreenSize());
    m_fx->setClearColour(irr::video::SColor(0));
    m_fx->enableDepthPass(true);

    std::vector<irr::video::ITexture*> textures = createTextures(device->getSceneManager());
    std::map<loader::TextureLayoutProxy::TextureKey, irr::video::SMaterial> materials = createMaterials(textures);
    std::vector<irr::video::SMaterial> coloredMaterials;
    for( int i = 0; i < 256; ++i )
    {
        irr::video::SMaterial result;
        // Set some defaults
        result.setTexture(0, createSolidColorTex(device->getSceneManager(), i));
        //result.BackfaceCulling = false;
        result.ColorMaterial = irr::video::ECM_DIFFUSE;
        result.Lighting = true;
        result.AmbientColor.set(0);

        coloredMaterials.emplace_back(result);
    }

    m_textureAnimator = std::make_shared<render::TextureAnimator>(m_animatedTextures);

    std::vector<irr::scene::SMesh*> staticMeshes;
    for( size_t i = 0; i < m_meshes.size(); ++i )
    {
        staticMeshes.emplace_back(m_meshes[i].createMesh(device->getSceneManager(), i, m_textureProxies, materials, coloredMaterials, *m_textureAnimator));
    }

    for( size_t i = 0; i < m_rooms.size(); ++i )
    {
        m_rooms[i].createSceneNode(device->getSceneManager(), i, *this, materials, textures, staticMeshes, *m_textureAnimator);
    }

    std::vector<irr::scene::ISkinnedMesh*> skinnedMeshes = createSkinnedMeshes(device->getSceneManager(), staticMeshes);

    m_lara = createItems(device->getSceneManager(), skinnedMeshes, textures);
    if(m_lara == nullptr)
        return;

    for( auto* ptr : staticMeshes )
        ptr->drop();

    for( auto& ptr : skinnedMeshes )
        ptr->drop();

    for(const auto& i : m_itemControllers)
        m_fx->addNodeToDepthPass(i.second->getSceneNode());

    if(device->getVideoDriver()->getDriverType() == irr::video::EDT_DIRECT3D9)
        m_fx->addPostProcessingEffectFromFile("shaders/black_depth.hlsl");
    else if(device->getVideoDriver()->getDriverType() == irr::video::EDT_OPENGL)
        m_fx->addPostProcessingEffectFromFile("shaders/black_depth.glsl");
    else
        throw std::runtime_error("Unsupported driver type");

    irr::scene::ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNode();
    m_cameraController = new engine::CameraController(this, m_lara, device->getSceneManager()->getVideoDriver(), camera);
    camera->addAnimator(m_cameraController);
    camera->bindTargetAndRotation(true);
    camera->setNearValue(10);
    camera->setFarValue(20480);
    camera->setFOV(irr::core::degToRad(80.0f));

    for(const loader::SoundSource& src : m_soundSources)
    {
        auto handle = playSound(src.sound_id, core::ExactTRCoordinates(src.position));
        handle->setLooping(true);
        m_audioDev.registerSource(handle);
    }
}

void Level::convertTexture(loader::ByteTexture& tex, loader::Palette& pal, loader::DWordTexture& dst)
{
    for( int y = 0; y < 256; y++ )
    {
        for( int x = 0; x < 256; x++ )
        {
            int col = tex.pixels[y][x];

            if( col > 0 )
                dst.pixels[y][x].set(0xff, pal.color[col].r, pal.color[col].g, pal.color[col].b);
            else
                dst.pixels[y][x].set(0);
        }
    }
}

void Level::convertTexture(loader::WordTexture& tex, loader::DWordTexture& dst)
{
    for( int y = 0; y < 256; y++ )
    {
        for( int x = 0; x < 256; x++ )
        {
            int col = tex.pixels[y][x];

            if( col & 0x8000 )
            {
                const uint32_t a = 0xff;
                const uint32_t r = ((col & 0x00007c00) >> 7);
                const uint32_t g = ((col & 0x000003e0) >> 2);
                const uint32_t b = ((col & 0x0000001f) << 3);
                dst.pixels[y][x].set(a, r, g, b);
            }
            else
            {
                dst.pixels[y][x].set(0);
            }
        }
    }
}

gsl::not_null<const loader::Sector*> Level::findFloorSectorWithClampedPosition(const core::TRCoordinates& position, gsl::not_null<gsl::not_null<const loader::Room*>*> room) const
{
    const loader::Sector* sector = nullptr;
    while( true )
    {
        sector = (*room)->findFloorSectorWithClampedIndex((position.X - (*room)->position.X) / loader::SectorSize, (position.Z - (*room)->position.Z) / loader::SectorSize);
        Expects(sector != nullptr);
        const auto portalTarget = sector->getPortalTarget(m_floorData);
        if( !portalTarget )
        {
            break;
        }

        BOOST_ASSERT(*portalTarget != 0xff && *portalTarget < m_rooms.size());
        *room = &m_rooms[*portalTarget];
    }

    Expects(sector != nullptr);
    if( sector->floorHeight * loader::QuarterSectorSize > position.Y )
    {
        while( sector->ceilingHeight * loader::QuarterSectorSize >= position.Y && sector->roomAbove != 0xff )
        {
            BOOST_ASSERT(sector->roomAbove < m_rooms.size());
            *room = &m_rooms[sector->roomAbove];
            sector = (*room)->getSectorByAbsolutePosition(position);
            Expects(sector != nullptr);
        }
    }
    else
    {
        while( sector->floorHeight * loader::QuarterSectorSize <= position.Y && sector->roomBelow != 0xff )
        {
            BOOST_ASSERT(sector->roomBelow < m_rooms.size());
            *room = &m_rooms[sector->roomBelow];
            sector = (*room)->getSectorByAbsolutePosition(position);
            Expects(sector != nullptr);
        }
    }

    return sector;
}

gsl::not_null<const loader::Room*> Level::findRoomForPosition(const core::ExactTRCoordinates& position, gsl::not_null<const loader::Room*> room) const
{
    const loader::Sector* sector = nullptr;
    while( true )
    {
        sector = room->findFloorSectorWithClampedIndex(gsl::narrow_cast<int>(position.X - room->position.X) / loader::SectorSize, gsl::narrow_cast<int>(position.Z - room->position.Z) / loader::SectorSize);
        Expects(sector != nullptr);
        const auto portalTarget = sector->getPortalTarget(m_floorData);
        if( !portalTarget )
        {
            break;
        }

        BOOST_ASSERT(*portalTarget != 0xff && *portalTarget < m_rooms.size());
        room = &m_rooms[*portalTarget];
    }

    Expects(sector != nullptr);
    if( sector->floorHeight * loader::QuarterSectorSize > position.Y )
    {
        while( sector->ceilingHeight * loader::QuarterSectorSize > position.Y && sector->roomAbove != 0xff )
        {
            BOOST_ASSERT(sector->roomAbove < m_rooms.size());
            room = &m_rooms[sector->roomAbove];
            sector = room->getSectorByAbsolutePosition(position.toInexact());
            Expects(sector != nullptr);
        }
    }
    else
    {
        while( sector->floorHeight * loader::QuarterSectorSize <= position.Y && sector->roomBelow != 0xff )
        {
            BOOST_ASSERT(sector->roomBelow < m_rooms.size());
            room = &m_rooms[sector->roomBelow];
            sector = room->getSectorByAbsolutePosition(position.toInexact());
            Expects(sector != nullptr);
        }
    }

    return room;
}

engine::ItemController* Level::getItemController(uint16_t id) const
{
    auto it = m_itemControllers.find(id);
    if(it == m_itemControllers.end())
        return nullptr;

    return it->second.get();
}

void Level::drawBars(irr::video::IVideoDriver* drv) const
{
    if(m_lara->isInWater())
    {
        const irr::s32 x0 = drv->getScreenSize().Width - 110;

        for(irr::s32 i = 7; i <= 13; ++i)
            drv->draw2DLine({ x0 - 1, i }, { x0 + 101, i }, m_palette->color[0].toSColor());
        drv->draw2DLine({ x0 - 2, 14 }, { x0 + 102, 14 }, m_palette->color[17].toSColor());
        drv->draw2DLine({ x0 + 102, 6 }, { x0 + 102, 14 }, m_palette->color[17].toSColor());
        drv->draw2DLine({ x0 + 102, 6 }, { x0 + 102, 14 }, m_palette->color[19].toSColor());
        drv->draw2DLine({ x0 - 2, 6 }, { x0 - 2, 14 }, m_palette->color[19].toSColor());

        const int p = irr::core::clamp(std::lround(m_lara->getAir() * 100 / 1800), 0L, 100L);
        if(p > 0)
        {
            drv->draw2DLine({ x0, 8 }, { x0 + p, 8 }, m_palette->color[32].toSColor());
            drv->draw2DLine({ x0, 9 }, { x0 + p, 9 }, m_palette->color[41].toSColor());
            drv->draw2DLine({ x0, 10 }, { x0 + p, 10 }, m_palette->color[32].toSColor());
            drv->draw2DLine({ x0, 11 }, { x0 + p, 11 }, m_palette->color[19].toSColor());
            drv->draw2DLine({ x0, 12 }, { x0 + p, 12 }, m_palette->color[21].toSColor());
        }
    }

    const irr::s32 x0 = 8;
    for(irr::s32 i = 7; i <= 13; ++i)
        drv->draw2DLine({ x0 - 1, i }, { x0 + 101, i }, m_palette->color[0].toSColor());
    drv->draw2DLine({ x0 - 2, 14 }, { x0 + 102, 14 }, m_palette->color[17].toSColor());
    drv->draw2DLine({ x0 + 102, 6 }, { x0 + 102, 14 }, m_palette->color[17].toSColor());
    drv->draw2DLine({ x0 + 102, 6 }, { x0 + 102, 14 }, m_palette->color[19].toSColor());
    drv->draw2DLine({ x0 - 2, 6 }, { x0 - 2, 14 }, m_palette->color[19].toSColor());

    const int p = irr::core::clamp(std::lround(m_lara->getHealth().getCurrentValue() * 100 / 1000), 0L, 100L);
    if(p > 0)
    {
        drv->draw2DLine({ x0, 8 }, { x0 + p, 8 }, m_palette->color[8].toSColor());
        drv->draw2DLine({ x0, 9 }, { x0 + p, 9 }, m_palette->color[11].toSColor());
        drv->draw2DLine({ x0, 10 }, { x0 + p, 10 }, m_palette->color[8].toSColor());
        drv->draw2DLine({ x0, 11 }, { x0 + p, 11 }, m_palette->color[6].toSColor());
        drv->draw2DLine({ x0, 12 }, { x0 + p, 12 }, m_palette->color[24].toSColor());
    }
}

engine::ItemController* level::Level::findControllerForNode(const irr::scene::ISceneNode* node)
{
    for(const auto& ctrl : m_itemControllers | boost::adaptors::map_values)
    {
        if(ctrl->getSceneNode() == node)
            return ctrl.get();
    }

    return nullptr;
}

void Level::triggerCdTrack(uint16_t trackId, uint16_t triggerArg, loader::TriggerType triggerType)
{
    if(trackId < 1 || trackId >= 64)
        return;

    if(trackId < 28)
    {
        triggerNormalCdTrack(trackId, triggerArg, triggerType);
        return;
    }

    if(trackId == 28)
    {
        if((m_cdTrackTriggerValues[trackId] & 0x100) != 0 && m_lara->getCurrentAnimState() == loader::LaraStateId::JumpUp)
            trackId = 29;
        triggerNormalCdTrack(trackId, triggerArg, triggerType);
        return;
    }

    if(trackId == 37 || trackId == 41)
    {
        if(m_lara->getCurrentAnimState() == loader::LaraStateId::Hang)
            triggerNormalCdTrack(trackId, triggerArg, triggerType);
        return;
    }

    if(trackId >= 29 && trackId <= 40)
    {
        triggerNormalCdTrack(trackId, triggerArg, triggerType);
        return;
    }

    if(trackId >= 42 && trackId <= 48)
    {
        if(trackId == 42 && (m_cdTrackTriggerValues[42] & 0x100) != 0 && m_lara->getCurrentAnimState() == loader::LaraStateId::Hang)
            trackId = 43;
        triggerNormalCdTrack(trackId, triggerArg, triggerType);
        return;
    }

    if(trackId == 49)
    {
        if(m_lara->getCurrentAnimState() == loader::LaraStateId::OnWaterStop)
            triggerNormalCdTrack(trackId, triggerArg, triggerType);
        return;
    }

    if(trackId == 50)
    {
        if((m_cdTrackTriggerValues[50] & 0x100) != 0)
        {
            if(++m_cdTrack50time == 120)
            {
                //! @todo End level
                m_cdTrack50time = 0;
            }
            triggerNormalCdTrack(trackId, triggerArg, triggerType);
            return;
        }

        if(m_lara->getCurrentAnimState() == loader::LaraStateId::OnWaterExit)
            triggerNormalCdTrack(trackId, triggerArg, triggerType);
        return;
    }

    if(trackId >= 51 && trackId <= 63)
    {
        triggerNormalCdTrack(trackId, triggerArg, triggerType);
        return;
    }
}

void Level::triggerNormalCdTrack(uint16_t trackId, uint16_t triggerArg, loader::TriggerType triggerType)
{
    if((m_cdTrackTriggerValues[trackId] & 0x100) != 0)
        return;

    const auto mask = triggerArg & 0x3e00;
    if(triggerType == loader::TriggerType::Switch)
        m_cdTrackTriggerValues[trackId] ^= mask;
    else if(triggerType == loader::TriggerType::AntiPad)
        m_cdTrackTriggerValues[trackId] &= ~mask;
    else
        m_cdTrackTriggerValues[trackId] |= mask;

    if((m_cdTrackTriggerValues[trackId] & 0x3e00) == 0x3e00)
    {
        m_cdTrackTriggerValues[trackId] |= (triggerArg & 0x100);

        if(m_activeCDTrack != trackId)
            playCdTrack(trackId);
    }
    else
    {
        stopCdTrack(trackId);
    }
}

void Level::playCdTrack(uint16_t trackId)
{
    if(trackId == 13)
    {
        m_lara->playSoundEffect(173);
        return;
    }

    if(trackId > 2 && trackId < 22)
        return;

    BOOST_LOG_TRIVIAL(debug) << "Stopping track #" << m_activeCDTrack;

    if(m_activeCDTrack >= 26 && m_activeCDTrack <= 56)
    {
        stopSoundEffect(m_activeCDTrack + 148);
    }
    else if(m_activeCDTrack > 0)
    {
        m_audioDev.removeStream(m_cdStream);
        m_cdStream.reset();
    }
    m_activeCDTrack = 0;

    BOOST_LOG_TRIVIAL(debug) << "Playing track #" << trackId;

    if(trackId >= 26 && trackId <= 56)
    {
        m_lara->playSoundEffect(trackId + 148);
        m_activeCDTrack = trackId;
        return;
    }

    if(trackId == 2)
    {
        trackId = 2;
    }
    else if(trackId >= 22 && trackId <= 25)
    {
        trackId -= 15;
    }
    else
    {
        if(trackId <= 56)
        {
            m_activeCDTrack = trackId;
            return;
        }
        trackId -= 54;
    }

    playStream(trackId);
    m_activeCDTrack = trackId;
}

void Level::stopCdTrack(uint16_t trackId)
{
    if(m_activeCDTrack == 0)
        return;

    if(m_activeCDTrack < 26 || m_activeCDTrack > 56)
    {
        m_audioDev.removeStream(m_cdStream);
        m_cdStream.reset();
    }
    else
    {
        stopSoundEffect(trackId + 148);
    }

    m_activeCDTrack = 0;
}

void Level::playStream(uint16_t trackId)
{
    static constexpr size_t DefaultBufferSize = 16384;

    m_audioDev.removeStream(m_cdStream);
    m_cdStream.reset();

    if(boost::filesystem::is_regular_file("data/tr1/audio/CDAUDIO.WAD"))
        m_cdStream = std::make_unique<audio::Stream>(std::make_unique<audio::WadStreamSource>("data/tr1/audio/CDAUDIO.WAD", trackId), DefaultBufferSize);
    else
        m_cdStream = std::make_unique<audio::Stream>(std::make_unique<audio::SndfileStreamSource>((boost::format("data/tr1/audio/%03d.ogg") % trackId).str()), DefaultBufferSize);

    m_audioDev.registerStream(m_cdStream);
}

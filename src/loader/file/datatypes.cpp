#include "datatypes.h"

#include "engine/engine.h"
#include "level/level.h"
#include "render/gl/vertexarray.h"
#include "render/scene/Material.h"
#include "render/scene/Sprite.h"
#include "render/scene/mesh.h"
#include "render/scene/names.h"
#include "render/textureanimator.h"
#include "util.h"
#include "util/helpers.h"

#include <boost/range/adaptors.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace loader
{
namespace file
{
namespace
{
#pragma pack(push, 1)

struct RenderVertex
{
    glm::vec3 position;
    glm::vec4 color{1.0f};
    glm::vec3 normal{0.0f};

    static const render::gl::StructureLayout<RenderVertex>& getLayout()
    {
        static const render::gl::StructureLayout<RenderVertex> layout{
            {VERTEX_ATTRIBUTE_POSITION_NAME, &RenderVertex::position},
            {VERTEX_ATTRIBUTE_NORMAL_NAME, &RenderVertex::normal},
            {VERTEX_ATTRIBUTE_COLOR_NAME, &RenderVertex::color}};

        return layout;
    }
};

#pragma pack(pop)

struct MeshPart
{
    using IndexBuffer = std::vector<uint16_t>;
    static_assert(std::is_unsigned<IndexBuffer::value_type>::value, "Index buffer entries must be unsigned");

    IndexBuffer indices;
    std::shared_ptr<render::scene::Material> material;
};

struct RenderModel
{
    std::vector<MeshPart> m_parts;

    std::shared_ptr<render::scene::Model>
        toModel(const gsl::not_null<std::shared_ptr<render::gl::StructuredArrayBuffer<RenderVertex>>>& vbuf,
                const gsl::not_null<std::shared_ptr<render::gl::StructuredArrayBuffer<glm::vec2>>>& uvBuf)
    {
        auto model = std::make_shared<render::scene::Model>();

        for(const MeshPart& localPart : m_parts)
        {
#ifndef NDEBUG
            for(auto idx : localPart.indices)
            {
                BOOST_ASSERT(idx < vbuf->size());
            }
#endif

            auto indexBuffer = std::make_shared<render::gl::ElementArrayBuffer<uint16_t>>();
            indexBuffer->setData(localPart.indices, ::gl::BufferUsageARB::StaticDraw);

            std::vector<gsl::not_null<std::shared_ptr<render::gl::ElementArrayBuffer<uint16_t>>>> indexBufs{
                indexBuffer};
            auto vBufs = std::make_tuple(vbuf, uvBuf);

            auto mesh = std::make_shared<render::scene::MeshImpl<uint16_t, RenderVertex, glm::vec2>>(
                std::make_shared<render::gl::VertexArray<uint16_t, RenderVertex, glm::vec2>>(
                    indexBufs, vBufs, localPart.material->getShaderProgram()->getHandle()));
            mesh->setMaterial(localPart.material);
            model->addMesh(mesh);
        }

        return model;
    }
};

template<size_t N>
core::TRVec getCenter(const std::array<VertexIndex, N>& faceVertices, const std::vector<RoomVertex>& roomVertices)
{
    core::TRVec s{0_len, 0_len, 0_len};
    for(const auto& v : faceVertices)
    {
        const auto& rv = v.from(roomVertices);
        s += rv.position;
    }

    return s / faceVertices.size();
}
} // namespace

void Room::createSceneNode(
    const size_t roomId,
    const level::Level& level,
    const std::map<TextureKey, gsl::not_null<std::shared_ptr<render::scene::Material>>>& materials,
    const std::map<TextureKey, gsl::not_null<std::shared_ptr<render::scene::Material>>>& waterMaterials,
    const std::vector<gsl::not_null<std::shared_ptr<render::scene::Model>>>& staticMeshModels,
    render::TextureAnimator& animator,
    const std::shared_ptr<render::scene::Material>& spriteMaterial,
    const std::shared_ptr<render::scene::Material>& portalMaterial)
{
    RenderModel renderModel;
    std::map<TextureKey, size_t> texBuffers;
    std::vector<RenderVertex> vbufData;
    std::vector<glm::vec2> uvCoordsData;

    const auto label = "Room:" + std::to_string(roomId);
    auto vbuf = std::make_shared<render::gl::StructuredArrayBuffer<RenderVertex>>(RenderVertex::getLayout(), label);

    static const render::gl::StructureLayout<glm::vec2> uvAttribs{
        {VERTEX_ATTRIBUTE_TEXCOORD_PREFIX_NAME, render::gl::StructureMember<glm::vec2>::Trivial{}}};
    auto uvCoords = std::make_shared<render::gl::StructuredArrayBuffer<glm::vec2>>(uvAttribs, label + "-uv");

    for(const QuadFace& quad : rectangles)
    {
        // discard water surface polygons
        const auto center = getCenter(quad.vertices, vertices);
        if(const auto sector = getSectorByRelativePosition(center))
        {
            if(sector->roomAbove != nullptr && sector->roomAbove->isWaterRoom() != isWaterRoom())
            {
                if(center.Y + position.Y == sector->ceilingHeight)
                    continue;
            }
            if(sector->roomBelow != nullptr && sector->roomBelow->isWaterRoom() != isWaterRoom())
            {
                if(center.Y + position.Y == sector->floorHeight)
                    continue;
            }
        }

        const TextureTile& tile = level.m_textureTiles.at(quad.tileId.get());

        if(texBuffers.find(tile.textureKey) == texBuffers.end())
        {
            texBuffers[tile.textureKey] = renderModel.m_parts.size();

            renderModel.m_parts.emplace_back();

            auto it = isWaterRoom() ? waterMaterials.at(tile.textureKey) : materials.at(tile.textureKey);
            renderModel.m_parts.back().material = it;
        }
        const auto partId = texBuffers[tile.textureKey];

        const auto firstVertex = vbufData.size();
        for(int i = 0; i < 4; ++i)
        {
            RenderVertex iv;
            iv.position = quad.vertices[i].from(vertices).position.toRenderSystem();
            iv.color = quad.vertices[i].from(vertices).color;
            uvCoordsData.push_back(tile.uvCoordinates[i].toGl());

            if(i <= 2)
            {
                static const int indices[3] = {0, 1, 2};
                iv.normal = generateNormal(quad.vertices[indices[(i + 0) % 3]].from(vertices).position,
                                           quad.vertices[indices[(i + 1) % 3]].from(vertices).position,
                                           quad.vertices[indices[(i + 2) % 3]].from(vertices).position);
            }
            else
            {
                static const int indices[3] = {0, 2, 3};
                iv.normal = generateNormal(quad.vertices[indices[(i + 0) % 3]].from(vertices).position,
                                           quad.vertices[indices[(i + 1) % 3]].from(vertices).position,
                                           quad.vertices[indices[(i + 2) % 3]].from(vertices).position);
            }

            vbufData.push_back(iv);
        }

        for(int i : {0, 1, 2, 0, 2, 3})
        {
            animator.registerVertex(quad.tileId, uvCoords, i, firstVertex + i);
            renderModel.m_parts[partId].indices.emplace_back(
                gsl::narrow<MeshPart::IndexBuffer::value_type>(firstVertex + i));
        }
    }
    for(const Triangle& tri : triangles)
    {
        // discard water surface polygons
        const auto center = getCenter(tri.vertices, vertices);
        if(const auto sector = getSectorByRelativePosition(center))
        {
            if(sector->roomAbove != nullptr && sector->roomAbove->isWaterRoom() != isWaterRoom())
            {
                if(center.Y + position.Y == sector->ceilingHeight)
                    continue;
            }
            if(sector->roomBelow != nullptr && sector->roomBelow->isWaterRoom() != isWaterRoom())
            {
                if(center.Y + position.Y == sector->floorHeight)
                    continue;
            }
        }

        const TextureTile& tile = level.m_textureTiles.at(tri.tileId.get());

        if(texBuffers.find(tile.textureKey) == texBuffers.end())
        {
            texBuffers[tile.textureKey] = renderModel.m_parts.size();

            renderModel.m_parts.emplace_back();

            auto it = isWaterRoom() ? waterMaterials.at(tile.textureKey) : materials.at(tile.textureKey);
            renderModel.m_parts.back().material = it;
        }
        const auto partId = texBuffers[tile.textureKey];

        const auto firstVertex = vbufData.size();
        for(int i = 0; i < 3; ++i)
        {
            RenderVertex iv;
            iv.position = tri.vertices[i].from(vertices).position.toRenderSystem();
            iv.color = tri.vertices[i].from(vertices).color;
            uvCoordsData.push_back(tile.uvCoordinates[i].toGl());

            static const int indices[3] = {0, 1, 2};
            iv.normal = generateNormal(tri.vertices[indices[(i + 0) % 3]].from(vertices).position,
                                       tri.vertices[indices[(i + 1) % 3]].from(vertices).position,
                                       tri.vertices[indices[(i + 2) % 3]].from(vertices).position);

            vbufData.push_back(iv);
        }

        for(int i : {0, 1, 2})
        {
            animator.registerVertex(tri.tileId, uvCoords, i, firstVertex + i);
            renderModel.m_parts[partId].indices.emplace_back(
                gsl::narrow<MeshPart::IndexBuffer::value_type>(firstVertex + i));
        }
    }

    vbuf->setData(vbufData, ::gl::BufferUsageARB::StaticDraw);
    uvCoords->setData(uvCoordsData, ::gl::BufferUsageARB::DynamicDraw);

    auto resModel = renderModel.toModel(vbuf, uvCoords);
    resModel->getRenderState().setCullFace(true);
    resModel->getRenderState().setCullFaceSide(::gl::CullFaceMode::Back);

    node = std::make_shared<render::scene::Node>("Room:" + std::to_string(roomId));
    node->setDrawable(resModel);
    node->addUniformSetter(
        "u_lightAmbient",
        [](const render::scene::Node& /*node*/, render::gl::ProgramUniform& uniform) { uniform.set(1.0f); });

    for(const RoomStaticMesh& sm : staticMeshes)
    {
        const auto idx = level.findStaticMeshIndexById(sm.meshId);
        if(idx < 0)
            continue;

        auto subNode = std::make_shared<render::scene::Node>("staticMesh");
        subNode->setDrawable(staticMeshModels.at(idx).get());
        subNode->setLocalMatrix(translate(glm::mat4{1.0f}, (sm.position - position).toRenderSystem())
                                * rotate(glm::mat4{1.0f}, toRad(sm.rotation), glm::vec3{0, -1, 0}));

        subNode->addUniformSetter(
            "u_lightAmbient",
            [brightness = sm.getBrightness()](const render::scene::Node& /*node*/,
                                              render::gl::ProgramUniform& uniform) { uniform.set(brightness); });
        addChild(node, subNode);
    }
    node->setLocalMatrix(translate(glm::mat4{1.0f}, position.toRenderSystem()));

    for(const SpriteInstance& spriteInstance : sprites)
    {
        BOOST_ASSERT(spriteInstance.vertex.get() < vertices.size());

        const Sprite& sprite = level.m_sprites.at(spriteInstance.id.get());

        const auto model = std::make_shared<render::scene::Sprite>(sprite.x0,
                                                                   -sprite.y0,
                                                                   sprite.x1,
                                                                   -sprite.y1,
                                                                   sprite.t0,
                                                                   sprite.t1,
                                                                   spriteMaterial,
                                                                   render::scene::Sprite::Axis::Y);

        auto spriteNode = std::make_shared<render::scene::Node>("sprite");
        spriteNode->setDrawable(model);
        const RoomVertex& v = vertices.at(spriteInstance.vertex.get());
        spriteNode->setLocalMatrix(translate(glm::mat4{1.0f}, v.position.toRenderSystem()));
        spriteNode->addUniformSetter(
            "u_diffuseTexture",
            [texture = sprite.texture](const render::scene::Node& /*node*/, render::gl::ProgramUniform& uniform) {
                uniform.set(*texture);
            });
        spriteNode->addUniformSetter(
            "u_lightAmbient",
            [brightness = v.getBrightness()](const render::scene::Node& /*node*/, render::gl::ProgramUniform& uniform) {
                uniform.set(brightness);
            });

        addChild(node, spriteNode);
    }
    for(auto& portal : portals)
        portal.buildMesh(portalMaterial);
}

core::BoundingBox StaticMesh::getCollisionBox(const core::TRVec& pos, const core::Angle& angle) const
{
    auto result = collision_box;

    const auto axis = axisFromAngle(angle, 45_deg);
    switch(*axis)
    {
    case core::Axis::PosZ:
        // nothing to do
        break;
    case core::Axis::PosX:
        result.min.X = collision_box.min.Z;
        result.max.X = collision_box.max.Z;
        result.min.Z = -collision_box.max.X;
        result.max.Z = -collision_box.min.X;
        break;
    case core::Axis::NegZ:
        result.min.X = -collision_box.max.X;
        result.max.X = -collision_box.min.X;
        result.min.Z = -collision_box.max.Z;
        result.max.Z = -collision_box.min.Z;
        break;
    case core::Axis::NegX:
        result.min.X = -collision_box.max.Z;
        result.max.X = -collision_box.min.Z;
        result.min.Z = collision_box.min.X;
        result.max.Z = collision_box.max.X;
        break;
    }

    result.min += pos;
    result.max += pos;
    return result;
}

void Room::patchHeightsForBlock(const engine::items::ItemNode& item, const core::Length& height)
{
    auto room = item.m_state.position.room;
    //! @todo Ugly const_cast
    auto groundSector = const_cast<Sector*>(loader::file::findRealFloorSector(item.m_state.position.position, &room));
    Expects(groundSector != nullptr);
    const auto topSector = loader::file::findRealFloorSector(
        item.m_state.position.position + core::TRVec{0_len, height - core::SectorSize, 0_len}, &room);

    if(groundSector->floorHeight == -core::HeightLimit)
    {
        groundSector->floorHeight = topSector->ceilingHeight + height;
    }
    else
    {
        groundSector->floorHeight = topSector->floorHeight + height;
        if(groundSector->floorHeight == topSector->ceilingHeight)
            groundSector->floorHeight = -core::HeightLimit;
    }

    Expects(groundSector->box != nullptr);

    if(groundSector->box->blockable)
        groundSector->box->blocked = (height < 0_len);
}

const Sector* findRealFloorSector(const core::TRVec& position, const gsl::not_null<gsl::not_null<const Room*>*>& room)
{
    const Sector* sector;
    // follow portals
    while(true)
    {
        sector = (*room)->findFloorSectorWithClampedIndex((position.X - (*room)->position.X) / core::SectorSize,
                                                          (position.Z - (*room)->position.Z) / core::SectorSize);
        if(sector->portalTarget == nullptr)
        {
            break;
        }

        *room = sector->portalTarget;
    }

    // go up/down until we are in the room that contains our coordinates
    Expects(sector != nullptr);
    if(position.Y >= sector->floorHeight)
    {
        while(position.Y >= sector->floorHeight && sector->roomBelow != nullptr)
        {
            *room = sector->roomBelow;
            sector = (*room)->getSectorByAbsolutePosition(position);
            if(sector == nullptr)
                return nullptr;
        }
    }
    else
    {
        while(position.Y < sector->ceilingHeight && sector->roomAbove != nullptr)
        {
            *room = sector->roomAbove;
            sector = (*room)->getSectorByAbsolutePosition(position);
            if(sector == nullptr)
                return nullptr;
        }
    }

    return sector;
}
} // namespace file
} // namespace loader

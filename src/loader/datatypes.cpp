#include "datatypes.h"

#include "level/level.h"
#include "render/textureanimator.h"
#include "util/vmath.h"
#include "loader/trx/trx.h"
#include <glm/gtc/type_ptr.hpp>

#include "CImg.h"

#include <boost/lexical_cast.hpp>
#include <boost/range/adaptors.hpp>
#include "util/md5.h"


namespace loader
{
    namespace
    {
#pragma pack(push,1)
        struct RenderVertex
        {
            glm::vec2 texcoord;
            glm::vec3 position;
            glm::vec4 color;
            glm::vec3 normal{0.0f};


            static const gameplay::VertexFormat& getFormat()
            {
                static const gameplay::VertexFormat::Element elems[4] = {
                    {gameplay::VertexFormat::TEXCOORD, 2},
                    {gameplay::VertexFormat::POSITION, 3},
                    {gameplay::VertexFormat::COLOR, 4},
                    {gameplay::VertexFormat::NORMAL, 3}
                };
                static const gameplay::VertexFormat fmt{elems, 4};

                Expects(fmt.getVertexSize() == sizeof(RenderVertex));

                return fmt;
            }
        };
#pragma pack(pop)

        struct MeshPart
        {
            using IndexBuffer = std::vector<uint16_t>;

            IndexBuffer indices;
            std::shared_ptr<gameplay::Material> material;
        };


        struct RenderModel
        {
            std::vector<MeshPart> m_parts;


            std::shared_ptr<gameplay::Model> toModel(const gsl::not_null<std::shared_ptr<gameplay::Mesh>>& mesh)
            {
                for( const MeshPart& localPart : m_parts )
                {
#ifndef NDEBUG
                    for( auto idx : localPart.indices )
                    BOOST_ASSERT(idx >= 0 && idx < mesh->getVertexCount());
#endif

                    auto part = mesh->addPart(gameplay::Mesh::PrimitiveType::TRIANGLES, gameplay::Mesh::IndexFormat::INDEX16, localPart.indices.size(), true);
                    part->setIndexData(localPart.indices.data(), 0, 0);
                }

                auto model = std::make_shared<gameplay::Model>();
                model->addMesh(mesh);

                for( size_t i = 0; i < m_parts.size(); ++i )
                {
                    mesh->getPart(i)->setMaterial(m_parts[i].material);
                }

                return model;
            }
        };
    }


    std::shared_ptr<gameplay::Node> Room::createSceneNode(gameplay::Game* game,
                                                          size_t roomId,
                                                          const level::Level& level,
                                                          const std::vector<std::shared_ptr<gameplay::Texture>>& textures,
                                                          const std::map<loader::TextureLayoutProxy::TextureKey, std::shared_ptr<gameplay::Material>>& materials,
                                                          const std::map<loader::TextureLayoutProxy::TextureKey, std::shared_ptr<gameplay::Material>>& waterMaterials,
                                                          const std::vector<std::shared_ptr<gameplay::Model>>& staticMeshes,
                                                          render::TextureAnimator& animator)
    {
        RenderModel renderModel;
        std::map<TextureLayoutProxy::TextureKey, size_t> texBuffers;
        std::vector<RenderVertex> vbuf;
        auto mesh = std::make_shared<gameplay::Mesh>(RenderVertex::getFormat(), vbuf.size(), true);

        for( const QuadFace& quad : rectangles )
        {
            const TextureLayoutProxy& proxy = level.m_textureProxies.at(quad.proxyId);

            if( texBuffers.find(proxy.textureKey) == texBuffers.end() )
            {
                texBuffers[proxy.textureKey] = renderModel.m_parts.size();
                renderModel.m_parts.emplace_back();
                auto it = isWaterRoom() ? waterMaterials.find(proxy.textureKey) : materials.find(proxy.textureKey);
                Expects(it != (isWaterRoom() ? waterMaterials.end() : materials.end()));
                renderModel.m_parts.back().material = it->second;
            }
            const auto partId = texBuffers[proxy.textureKey];

            const auto firstVertex = vbuf.size();
            for( int i = 0; i < 4; ++i )
            {
                RenderVertex iv;
                iv.position = vertices[quad.vertices[i]].position.toRenderSystem();
                iv.color = vertices[quad.vertices[i]].color;
                iv.texcoord = proxy.uvCoordinates[i].toGl();
                vbuf.push_back(iv);
            }

            animator.registerVertex(quad.proxyId, {mesh, partId}, 0, firstVertex + 0);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 0);
            animator.registerVertex(quad.proxyId, {mesh, partId}, 1, firstVertex + 1);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 1);
            animator.registerVertex(quad.proxyId, {mesh, partId}, 2, firstVertex + 2);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 2);
            animator.registerVertex(quad.proxyId, {mesh, partId}, 0, firstVertex + 0);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 0);
            animator.registerVertex(quad.proxyId, {mesh, partId}, 2, firstVertex + 2);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 2);
            animator.registerVertex(quad.proxyId, {mesh, partId}, 3, firstVertex + 3);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 3);
        }
        for( const Triangle& tri : triangles )
        {
            const TextureLayoutProxy& proxy = level.m_textureProxies.at(tri.proxyId);

            if( texBuffers.find(proxy.textureKey) == texBuffers.end() )
            {
                texBuffers[proxy.textureKey] = renderModel.m_parts.size();
                renderModel.m_parts.emplace_back();
                auto it = isWaterRoom() ? waterMaterials.find(proxy.textureKey) : materials.find(proxy.textureKey);
                Expects(it != (isWaterRoom() ? waterMaterials.end() : materials.end()));
                renderModel.m_parts.back().material = it->second;
            }
            const auto partId = texBuffers[proxy.textureKey];

            const auto firstVertex = vbuf.size();
            for( int i = 0; i < 3; ++i )
            {
                RenderVertex iv;
                iv.position = vertices[tri.vertices[i]].position.toRenderSystem();
                iv.color = vertices[tri.vertices[i]].color;
                iv.texcoord = proxy.uvCoordinates[i].toGl();
                vbuf.push_back(iv);
            }

            animator.registerVertex(tri.proxyId, {mesh, partId}, 0, firstVertex + 0);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 0);
            animator.registerVertex(tri.proxyId, {mesh, partId}, 1, firstVertex + 1);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 1);
            animator.registerVertex(tri.proxyId, {mesh, partId}, 2, firstVertex + 2);
            renderModel.m_parts[partId].indices.emplace_back(firstVertex + 2);
        }

        mesh->rebuild(reinterpret_cast<float*>(vbuf.data()), vbuf.size());
        auto resModel = renderModel.toModel(mesh);
        node = std::make_shared<gameplay::Node>("Room:" + boost::lexical_cast<std::string>(roomId));
        node->setDrawable(resModel);
        node->addMaterialParameterSetter("u_lightPosition", [](const gameplay::Node& node, const std::shared_ptr<gameplay::ShaderProgram>& shaderProgram, const std::shared_ptr<gameplay::Uniform>& uniform)
        {
            shaderProgram->setValue(*uniform, glm::vec3{0.0f});
        });
        node->addMaterialParameterSetter("u_baseLight", [](const gameplay::Node& node, const std::shared_ptr<gameplay::ShaderProgram>& shaderProgram, const std::shared_ptr<gameplay::Uniform>& uniform)
        {
            shaderProgram->setValue(*uniform, 1.0f);
        });
        node->addMaterialParameterSetter("u_baseLightDiff", [](const gameplay::Node& node, const std::shared_ptr<gameplay::ShaderProgram>& shaderProgram, const std::shared_ptr<gameplay::Uniform>& uniform)
        {
            shaderProgram->setValue(*uniform, 1.0f);
        });

        for( const RoomStaticMesh& sm : this->staticMeshes )
        {
            auto idx = level.findStaticMeshIndexById(sm.meshId);
            BOOST_ASSERT(idx >= 0);
            BOOST_ASSERT(static_cast<size_t>(idx) < staticMeshes.size());
            auto subNode = std::make_shared<gameplay::Node>("");
            subNode->setDrawable(staticMeshes[idx]);
            subNode->setLocalMatrix(glm::translate(glm::mat4{1.0f}, (sm.position - position).toRenderSystem()) * glm::rotate(glm::mat4{1.0f}, util::auToRad(sm.rotation), glm::vec3{0,-1,0}));

            float brightness = 1 - (sm.darkness - 4096) / 8192.0f;

            subNode->addMaterialParameterSetter("u_baseLight", [brightness](const gameplay::Node& node, const std::shared_ptr<gameplay::ShaderProgram>& shaderProgram, const std::shared_ptr<gameplay::Uniform>& uniform)
            {
                shaderProgram->setValue(*uniform, brightness);
            });
            subNode->addMaterialParameterSetter("u_baseLightDiff", [](const gameplay::Node& node, const std::shared_ptr<gameplay::ShaderProgram>& shaderProgram, const std::shared_ptr<gameplay::Uniform>& uniform)
            {
                shaderProgram->setValue(*uniform, 0.0f);
            });
            subNode->addMaterialParameterSetter("u_lightPosition", [](const gameplay::Node& node, const std::shared_ptr<gameplay::ShaderProgram>& shaderProgram, const std::shared_ptr<gameplay::Uniform>& uniform)
            {
                shaderProgram->setValue(*uniform, glm::vec3{ std::numeric_limits<float>::quiet_NaN() });
            });
            node->addChild(subNode);
        }
        node->setLocalMatrix(glm::translate(glm::mat4{1.0f}, position.toRenderSystem()));

        for( const Sprite& sprite : sprites )
        {
            BOOST_ASSERT(sprite.vertex < vertices.size());
            BOOST_ASSERT(sprite.texture < level.m_spriteTextures.size());

            const SpriteTexture& tex = level.m_spriteTextures[sprite.texture];

            auto spriteNode = std::make_shared<gameplay::Sprite>(game, textures[tex.texture], tex.right_side - tex.left_side + 1, tex.bottom_side - tex.top_side + 1, tex.buildSourceRectangle());
            spriteNode->setBlendMode(gameplay::Sprite::BLEND_ADDITIVE);

            auto n = std::make_shared<gameplay::Node>("");
            n->setDrawable(spriteNode);
            n->setLocalMatrix(glm::translate(glm::mat4{1.0f}, (vertices[sprite.vertex].position - core::TRCoordinates{0, tex.bottom_side / 2, 0}).toRenderSystem()));

            node->addChild(n);
        }

        return node;
    }


    std::shared_ptr<gameplay::Image> DWordTexture::toImage(trx::Glidos* glidos) const
    {
        if(glidos == nullptr)
            return std::make_shared<gameplay::Image>(256, 256, &pixels[0][0]);

        BOOST_LOG_TRIVIAL(info) << "Upgrading texture " << md5 << "...";

        constexpr int Resolution = 2048;
        constexpr int Scale = Resolution / 256;


        cimg_library::CImg<float> original(glm::value_ptr(pixels[0][0]), 4, 256, 256, 1, false);
        // un-interleave
        original.permute_axes("yzcx");
        BOOST_ASSERT(original.width() == 256 && original.height() == 256 && original.spectrum() == 4);
        original.resize(Resolution, Resolution, 1, 4, 6);
        BOOST_ASSERT(original.width() == Resolution && original.height() == Resolution && original.spectrum() == 4);

        auto mappings = glidos->getMappingsForTexture(md5);

        for(const auto& mapping : mappings)
        {
            BOOST_LOG_TRIVIAL(info) << "  - Loading " << mapping.second << " into " << mapping.first;
            if(!boost::filesystem::is_regular_file(mapping.second))
            {
                BOOST_LOG_TRIVIAL(warning) << "    File not found";
                continue;
            }

            cimg_library::CImg<float> srcImage(mapping.second.string().c_str());
            srcImage /= 255;

            if(srcImage.spectrum() == 3)
            {
                srcImage.channels(0, 3);
                BOOST_ASSERT(srcImage.spectrum() == 4);
                srcImage.get_shared_channel(3).fill(1);
            }

            if(srcImage.spectrum() != 4)
            {
                BOOST_THROW_EXCEPTION(std::runtime_error("Can only use RGB and RGBA images"));
            }

            srcImage.resize(mapping.first.getWidth() * Scale, mapping.first.getHeight() * Scale, 1, 4, 6);
            const auto x0 = mapping.first.getX0()*Scale;
            const auto y0 = mapping.first.getY0()*Scale;
            for(int x=0; x<srcImage.width(); ++x)
            {
                for(int y = 0; y<srcImage.height(); ++y)
                {
                    for(int c=0; c<4; ++c)
                    {
                        original(x + x0, y + y0, 0, c) = srcImage(x, y, 0, c);
                    }
                }
            }
        }

        // interleave
        original.permute_axes("cxyz");

        return std::make_shared<gameplay::Image>(Resolution, Resolution, reinterpret_cast<const glm::vec4*>(original.data()));
    }


    std::shared_ptr<gameplay::Texture> DWordTexture::toTexture(trx::Glidos* glidos) const
    {
        return std::make_shared<gameplay::Texture>(toImage(glidos), false);
    }


    gameplay::BoundingBox StaticMesh::getCollisionBox(const core::TRCoordinates& pos, core::Angle angle) const
    {
        auto result = collision_box;

        const auto axis = core::axisFromAngle(angle, 45_deg);
        switch( *axis )
        {
            case core::Axis::PosZ:
                // nothing to do
                break;
            case core::Axis::PosX:
                std::swap(result.min.x, result.min.z);
                result.min.z *= -1;
                std::swap(result.max.x, result.max.z);
                result.max.z *= -1;
                break;
            case core::Axis::NegZ:
                result.min.x *= -1;
                result.min.z *= -1;
                result.max.x *= -1;
                result.max.z *= -1;
                break;
            case core::Axis::NegX:
                std::swap(result.min.x, result.min.z);
                result.min.x *= -1;
                std::swap(result.max.x, result.max.z);
                result.max.x *= -1;
                break;
        }

        result.min += glm::vec3(pos.X, pos.Y, pos.Z);
        result.max += glm::vec3(pos.X, pos.Y, pos.Z);
        return result;
    }


    void Room::patchHeightsForBlock(const engine::items::ItemNode& ctrl, int height)
    {
        core::RoomBoundPosition pos = ctrl.getRoomBoundPosition();
        //! @todo Ugly const_cast
        auto groundSector = const_cast<loader::Sector*>(ctrl.getLevel().findFloorSectorWithClampedPosition(pos).get());
        pos.position.Y += height - loader::SectorSize;
        const auto topSector = ctrl.getLevel().findFloorSectorWithClampedPosition(pos);

        const auto q = height / loader::QuarterSectorSize;
        if( groundSector->floorHeight == -127 )
        {
            groundSector->floorHeight = topSector->ceilingHeight + q;
        }
        else
        {
            groundSector->floorHeight += q;
            if( groundSector->floorHeight == topSector->ceilingHeight )
                groundSector->floorHeight = -127;
        }

        if( groundSector->boxIndex == 0xffff )
            return;

        //! @todo Ugly const_cast
        loader::Box& box = const_cast<loader::Box&>(ctrl.getLevel().m_boxes[groundSector->boxIndex]);
        if( (box.overlap_index & 0x8000) == 0 )
            return;

        if( height >= 0 )
            box.overlap_index &= ~0x4000;
        else
            box.overlap_index |= 0x4000;
    }
}

#include "render.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <set>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <boost/log/trivial.hpp>

#include "bsp_tree.h"
#include "engine/engine.h"
#include "engine/system.h"
#include "gui/console.h"
#include "shader_description.h"
#include "shader_manager.h"
#include "util/vmath.h"
#include "world/animation/animation.h"
#include "world/camera.h"
#include "world/character.h"
#include "world/core/basemesh.h"
#include "world/core/frustum.h"
#include "world/core/light.h"
#include "world/core/orientedboundingbox.h"
#include "world/core/polygon.h"
#include "world/core/sprite.h"
#include "world/entity.h"
#include "world/hair.h"
#include "world/portal.h"
#include "world/resource.h"
#include "world/room.h"
#include "world/skeletalmodel.h"
#include "world/staticmesh.h"
#include "world/world.h"

namespace render
{

Render renderer;
DynamicBSP render_dBSP;
extern RenderDebugDrawer debugDrawer;

using gui::Console;

void Render::initGlobals()
{
    m_settings = RenderSettings();
}

void Render::doShaders()
{
    m_shaderManager.reset(new ShaderManager());
}

void Render::init()
{
    m_blocked = true;
    m_cam = nullptr;

    m_renderList.clear();

    m_world = nullptr;

    m_drawWire = false;
    m_drawRoomBoxes = false;
    m_drawBoxes = false;
    m_drawPortals = false;
    m_drawNormals = false;
    m_drawAxis = false;
    m_skipRoom = false;
    m_skipStatic = false;
    m_skipEntities = false;
    m_drawAllModels = false;
    m_drawDummyStatics = false;
    m_drawColl = false;
    m_drawSkybox = false;
    m_drawPoints = false;
}

void Render::empty()
{
    m_world = nullptr;

    m_renderList.clear();

    m_shaderManager.reset();
}

void Render::renderSkyBox()
{
    if(m_drawSkybox && (m_world != nullptr) && (m_world->sky_box != nullptr))
    {
        glDepthMask(GL_FALSE);
        glm::mat4 tr = glm::translate(glm::mat4(1.0f), m_cam->getPosition() + m_world->sky_box->animations.front().getInitialBoneKeyFrame().offset);
        tr *= glm::mat4_cast( m_world->sky_box->animations[0].getInitialBoneKeyFrame().qrotate );
        const glm::mat4 fullView = m_cam->getViewProjection() * tr;

        UnlitTintedShaderDescription *shader = m_shaderManager->getStaticMeshShader();
        glUseProgram(shader->program);
        glUniformMatrix4fv(shader->model_view_projection, 1, false, glm::value_ptr(fullView));
        glUniform1i(shader->sampler, 0);
        glm::vec4 tint = { 1, 1, 1, 1 };
        glUniform4fv(shader->tint_mult, 1, glm::value_ptr(tint));

        renderMesh(m_world->sky_box->meshes[0].mesh_base);
        glDepthMask(GL_TRUE);
    }
}

/**
 * Opaque meshes drawing
 */
void Render::renderMesh(const std::shared_ptr<world::core::BaseMesh>& mesh)
{
    if(!mesh->m_allAnimatedElements.empty())
    {
        // Respecify the tex coord buffer
        glBindBuffer(GL_ARRAY_BUFFER, mesh->m_animatedVboTexCoordArray);
        // Tell OpenGL to discard the old values
        glBufferData(GL_ARRAY_BUFFER, mesh->m_animatedVertices.size() * sizeof(glm::vec2), nullptr, GL_STREAM_DRAW);
        // Get writable data (to avoid copy)
        glm::vec2 *data = static_cast<glm::vec2*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

        size_t offset = 0;
        for(const world::core::Polygon& p : mesh->m_polygons)
        {
            if(p.anim_id == 0 || p.isBroken())
            {
                continue;
            }

            world::animation::AnimSeq* seq = &engine::engine_world.anim_sequences[p.anim_id - 1];

            uint16_t frame = (seq->current_frame + p.frame_offset) % seq->frames.size();
            world::animation::TexFrame* tf = &seq->frames[frame];
            for(const world::core::Vertex& vert : p.vertices)
            {
                const auto& v = vert.tex_coord;
                data[offset][0] = tf->mat[0 + 0 * 2] * v[0] + tf->mat[0 + 1 * 2] * v[1] + tf->move[0];
                data[offset][1] = tf->mat[1 + 0 * 2] * v[0] + tf->mat[1 + 1 * 2] * v[1] + tf->move[1];

                ++offset;
            }
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);

        if(mesh->m_animatedElementCount > 0)
        {
            mesh->m_animatedVertexArray->bind();

            //! @bug textures[0] only works if all animated textures are on the first page
            glBindTexture(GL_TEXTURE_2D, m_world->textures[0]);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh->m_animatedElementCount), GL_UNSIGNED_INT, nullptr);
        }
    }

    if(!mesh->m_vertices.empty())
    {
        mesh->m_mainVertexArray->bind();

        const uint32_t* const elementsbase = nullptr;

        size_t offset = 0;
        for(uint32_t texture = 0; texture < mesh->m_texturePageCount; texture++)
        {
            if(mesh->m_elementsPerTexture[texture] == 0)
            {
                continue;
            }

            glBindTexture(GL_TEXTURE_2D, m_world->textures[texture]);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh->m_elementsPerTexture[texture]), GL_UNSIGNED_INT, elementsbase + offset);
            offset += mesh->m_elementsPerTexture[texture];
        }
    }
}

/**
 * draw transparency polygons
 */
void Render::renderPolygonTransparency(loader::BlendingMode currentTransparency, const BSPFaceRef& bsp_ref, const UnlitTintedShaderDescription *shader)
{
    // Blending mode switcher.
    // Note that modes above 2 aren't explicitly used in TR textures, only for
    // internal particle processing. Theoretically it's still possible to use
    // them if you will force type via TRTextur utility.
    if(currentTransparency != bsp_ref.polygon->polygon->blendMode)
    {
        currentTransparency = bsp_ref.polygon->polygon->blendMode;
        switch(bsp_ref.polygon->polygon->blendMode)
        {
            case loader::BlendingMode::Multiply:                                    // Classic PC alpha
                glBlendFunc(GL_ONE, GL_ONE);
                break;

            case loader::BlendingMode::InvertSrc:                                  // Inversion by src (PS darkness) - SAME AS IN TR3-TR5
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                break;

            case loader::BlendingMode::InvertDst:                                 // Inversion by dest
                glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
                break;

            case loader::BlendingMode::Screen:                                      // Screen (smoke, etc.)
                glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);
                break;

            case loader::BlendingMode::AnimatedTexture:
                glBlendFunc(GL_ONE, GL_ZERO);
                break;

            default:                                             // opaque animated textures case
                break;
        };
    }

    glm::mat4 mvp = m_cam->getViewProjection() * bsp_ref.transform;

    glUniformMatrix4fv(shader->model_view_projection, 1, false, glm::value_ptr(mvp));

    bsp_ref.polygon->used_vertex_array->bind();
    glBindTexture(GL_TEXTURE_2D, m_world->textures[bsp_ref.polygon->polygon->tex_index]);

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(bsp_ref.polygon->count), GL_UNSIGNED_INT, reinterpret_cast<GLvoid *>(sizeof(GLuint) * bsp_ref.polygon->firstIndex));
}

void Render::renderBSPFrontToBack(loader::BlendingMode currentTransparency, const std::unique_ptr<BSPNode>& root, const UnlitTintedShaderDescription *shader)
{
    if(root->plane.distance(engine::engine_camera.getPosition()) >= 0)
    {
        if(root->front != nullptr)
        {
            renderBSPFrontToBack(currentTransparency, root->front, shader);
        }

        for(const BSPFaceRef& p : root->polygons_front)
        {
            renderPolygonTransparency(currentTransparency, p, shader);
        }
        for(const BSPFaceRef& p : root->polygons_back)
        {
            renderPolygonTransparency(currentTransparency, p, shader);
        }

        if(root->back != nullptr)
        {
            renderBSPFrontToBack(currentTransparency, root->back, shader);
        }
    }
    else
    {
        if(root->back != nullptr)
        {
            renderBSPFrontToBack(currentTransparency, root->back, shader);
        }

        for(const BSPFaceRef& p : root->polygons_back)
        {
            renderPolygonTransparency(currentTransparency, p, shader);
        }
        for(const BSPFaceRef& p : root->polygons_front)
        {
            renderPolygonTransparency(currentTransparency, p, shader);
        }

        if(root->front != nullptr)
        {
            renderBSPFrontToBack(currentTransparency, root->front, shader);
        }
    }
}

void Render::renderBSPBackToFront(loader::BlendingMode currentTransparency, const std::unique_ptr<BSPNode>& root, const UnlitTintedShaderDescription *shader)
{
    if(root->plane.distance(engine::engine_camera.getPosition()) >= 0)
    {
        if(root->back != nullptr)
        {
            renderBSPBackToFront(currentTransparency, root->back, shader);
        }

        for(const BSPFaceRef& p : root->polygons_back)
        {
            renderPolygonTransparency(currentTransparency, p, shader);
        }
        for(const BSPFaceRef& p : root->polygons_front)
        {
            renderPolygonTransparency(currentTransparency, p, shader);
        }

        if(root->front != nullptr)
        {
            renderBSPBackToFront(currentTransparency, root->front, shader);
        }
    }
    else
    {
        if(root->front != nullptr)
        {
            renderBSPBackToFront(currentTransparency, root->front, shader);
        }

        for(const BSPFaceRef& p : root->polygons_front)
        {
            renderPolygonTransparency(currentTransparency, p, shader);
        }
        for(const BSPFaceRef& p : root->polygons_back)
        {
            renderPolygonTransparency(currentTransparency, p, shader);
        }

        if(root->back != nullptr)
        {
            renderBSPBackToFront(currentTransparency, root->back, shader);
        }
    }
}

/**
 * skeletal model drawing
 */
void Render::renderSkeletalModel(const LitShaderDescription *shader, world::animation::Skeleton *skeleton, const glm::mat4& mvMatrix, const glm::mat4& mvpMatrix)
{
    for(const world::animation::Bone& bone : skeleton->getBones())
    {
        glUniformMatrix4fv(shader->model_view, 1, false, glm::value_ptr(mvMatrix * bone.full_transform));

        glUniformMatrix4fv(shader->model_view_projection, 1, false, glm::value_ptr(mvpMatrix * bone.full_transform));

        renderMesh(bone.mesh);
        if(bone.mesh_slot)
        {
            renderMesh(bone.mesh_slot);
        }
    }
}

void Render::renderSkeletalModelSkin(const LitShaderDescription *shader, world::Entity* ent, const glm::mat4& mvMatrix)
{
    glUniformMatrix4fv(shader->projection, 1, false, glm::value_ptr(m_cam->getProjection()));

    for(const world::animation::Bone& bone : ent->m_skeleton.getBones())
    {
        glm::mat4 transforms[2];
        transforms[0] = mvMatrix * bone.full_transform;

        // Calculate parent transform
        const glm::mat4& parentTransform = bone.parent ? bone.parent->full_transform : ent->m_transform;

        glm::mat4 secondTransform = parentTransform * glm::translate(glm::mat4(1.0f), bone.offset);

        transforms[1] = mvMatrix * secondTransform;
        glUniformMatrix4fv(shader->model_view, 2, false, glm::value_ptr(transforms[0]));

        if(bone.mesh_skin)
        {
            renderMesh(bone.mesh_skin);
        }
    }
}

void Render::renderDynamicEntitySkin(const LitShaderDescription *shader, world::Entity* ent)
{
    glUniformMatrix4fv(shader->projection, 1, false, glm::value_ptr(m_cam->getProjection()));

    for(const world::animation::Bone& bone : ent->m_skeleton.getBones())
    {
        glm::mat4 mvTransforms[2];

        glm::mat4 tr0(1.0f);
        bone.bt_body->getWorldTransform().getOpenGLMatrix(glm::value_ptr(tr0));

        mvTransforms[0] = m_cam->getView() * tr0;

        // Calculate parent transform
        glm::mat4 tr1;
        if(bone.parent)
            bone.parent->bt_body->getWorldTransform().getOpenGLMatrix(glm::value_ptr(tr1));
        else
            tr1 = ent->m_transform;

        glm::mat4 secondTransform = glm::translate(tr1, bone.offset);
        mvTransforms[1] = m_cam->getView() * secondTransform;

        glUniformMatrix4fv(shader->model_view, 2, false, glm::value_ptr(mvTransforms[0]));

        if(bone.mesh_skin)
        {
            renderMesh(bone.mesh_skin);
        }
    }
}

/**
 * Sets up the light calculations for the given entity based on its current
 * room. Returns the used shader, which will have been made current already.
 */
const LitShaderDescription *Render::setupEntityLight(world::Entity* entity, bool skin)
{
    // Calculate lighting
    if(!entity->getRoom())
    {
        const LitShaderDescription *shader = m_shaderManager->getEntityShader(0, skin);
        glUseProgram(shader->program);
        return shader;
    }

    glm::vec4 ambient_component( entity->getRoom()->m_ambientLighting, 1.0f );

    if(entity->getRoom()->m_flags & TR_ROOM_FLAG_WATER)
    {
        ambient_component *= engine::engine_world.calculateWaterTint();
    }

    GLenum current_light_number = 0;

    std::array<glm::vec3, EntityShaderLightsLimit> positions;

    std::array<glm::vec4, EntityShaderLightsLimit> colors;
    std::fill(colors.begin(), colors.end(), glm::vec4(0));

    std::array<glm::float_t, EntityShaderLightsLimit> innerRadiuses;
    std::fill(innerRadiuses.begin(), innerRadiuses.end(), 0);

    std::array<glm::float_t, EntityShaderLightsLimit> outerRadiuses;
    std::fill(outerRadiuses.begin(), outerRadiuses.end(), 0);

    for(size_t i = 0; i < entity->getRoom()->m_lights.size() && current_light_number < EntityShaderLightsLimit; i++)
    {
        world::core::Light *current_light = &entity->getRoom()->m_lights[i];

        glm::vec3 xyz = glm::vec3(entity->m_transform[3]) - current_light->position;
        glm::float_t distance = glm::length(xyz);

        // Find color
        colors[current_light_number] = {
            glm::clamp(current_light->colour[0], 0.0f, 1.0f),
            glm::clamp(current_light->colour[1], 0.0f, 1.0f),
            glm::clamp(current_light->colour[2], 0.0f, 1.0f),
            glm::clamp(current_light->colour[3], 0.0f, 1.0f)
        };

        if(entity->getRoom()->m_flags & TR_ROOM_FLAG_WATER)
        {
            colors[current_light_number] *= engine::engine_world.calculateWaterTint();
        }

        // Find position
        positions[current_light_number] = glm::vec3(m_cam->getView() * glm::vec4(current_light->position, 1.0f));

        // Find fall-off
        if(current_light->light_type == loader::LightType::Sun)
        {
            innerRadiuses[current_light_number] = 1e20f;
            outerRadiuses[current_light_number] = 1e21f;
            current_light_number++;
        }
        else if(distance <= current_light->outer + 1024.0f && (current_light->light_type == loader::LightType::Point || current_light->light_type == loader::LightType::Shadow))
        {
            innerRadiuses[current_light_number] = glm::abs(current_light->inner);
            outerRadiuses[current_light_number] = glm::abs(current_light->outer);
            current_light_number++;
        }
    }

    const LitShaderDescription *shader = m_shaderManager->getEntityShader(current_light_number, skin);
    glUseProgram(shader->program);
    glUniform4fv(shader->light_ambient, 1, glm::value_ptr(ambient_component));
    glUniform4fv(shader->light_color, current_light_number, reinterpret_cast<const glm::float_t*>(colors.data()));
    glUniform3fv(shader->light_position, current_light_number, reinterpret_cast<const glm::float_t*>(positions.data()));
    glUniform1fv(shader->light_inner_radius, current_light_number, innerRadiuses.data());
    glUniform1fv(shader->light_outer_radius, current_light_number, outerRadiuses.data());
    return shader;
}

void Render::renderEntity(world::Entity* entity)
{
    if(!m_drawAllModels && (entity->m_wasRendered || !entity->m_visible))
        return;

    // Calculate lighting
    const LitShaderDescription *shader = setupEntityLight(entity, false);

    if(entity->m_skeleton.getModel() && !entity->m_skeleton.getModel()->animations.empty())
    {
        // base frame offset
        if(entity->m_typeFlags & ENTITY_TYPE_DYNAMIC)
        {
            renderDynamicEntity(shader, entity);
            ///@TODO: where I need to do bf skinning matrices update? this time ragdoll update function calculates these matrices;
            if(entity->m_skeleton.getBones().front().mesh_skin)
            {
                const LitShaderDescription *skinShader = setupEntityLight(entity, true);
                renderDynamicEntitySkin(skinShader, entity);
            }
        }
        else
        {
            glm::mat4 scaledTransform = glm::scale(entity->m_transform, entity->m_scaling);
            glm::mat4 subModelView = m_cam->getView() * scaledTransform;
            glm::mat4 subModelViewProjection = m_cam->getViewProjection() * scaledTransform;
            renderSkeletalModel(shader, &entity->m_skeleton, subModelView, subModelViewProjection);
            if(entity->m_skeleton.getBones().front().mesh_skin)
            {
                const LitShaderDescription *skinShader = setupEntityLight(entity, true);
                renderSkeletalModelSkin(skinShader, entity, subModelView);
            }
        }
    }
}

void Render::renderDynamicEntity(const LitShaderDescription *shader, world::Entity* entity)
{
    for(const world::animation::Bone& bone : entity->m_skeleton.getBones())
    {
        glm::mat4 tr(1.0f);
        bone.bt_body->getWorldTransform().getOpenGLMatrix(glm::value_ptr(tr));
        glm::mat4 mvTransform = m_cam->getView() * tr;

        glUniformMatrix4fv(shader->model_view, 1, false, glm::value_ptr(mvTransform));

        glm::mat4 mvpTransform = m_cam->getViewProjection() * tr;
        glUniformMatrix4fv(shader->model_view_projection, 1, false, glm::value_ptr(mvpTransform));

        renderMesh(bone.mesh);
        if(bone.mesh_slot)
        {
            renderMesh(bone.mesh_slot);
        }
    }
}

///@TODO: add joint between hair and head; do Lara's skinning by vertex position copy (no inverse matrices and other) by vertex map;
void Render::renderHair(std::shared_ptr<world::Character> entity)
{
    if(!entity || entity->m_hairs.empty())
        return;

    // Calculate lighting
    const LitShaderDescription *shader = setupEntityLight(entity.get(), true);

    for(const std::shared_ptr<world::Hair>& hair : entity->m_hairs)
    {
        // First: Head attachment
        glm::mat4 globalHead(entity->m_transform * entity->m_skeleton.getBones()[hair->m_ownerBody].full_transform);
        glm::mat4 globalAttachment = globalHead * hair->m_ownerBodyHairRoot;

        static constexpr int MatrixCount = 10;

        glm::mat4 hairModelToGlobalMatrices[MatrixCount];
        hairModelToGlobalMatrices[0] = m_cam->getView() * globalAttachment;

        // Then: Individual hair pieces
        size_t i = 0;
        for(const world::HairElement& hairElement : hair->m_elements)
        {
            /*
             * Definitions: x_o - as in original file. x_h - as in hair model
             * (translated)
             * M_ho - translation matrix. x_g = global position (before modelview)
             * M_go - global position
             *
             * We know:
             * x_h = M_ho * x_o
             * x_g = M_go * x_o
             * We want:
             * M_hg so that x_g = M_gh * x_m
             * We have: M_oh, M_g
             *
             * x_h = M_ho * x_o => x_o = M_oh^-1 * x_h
             * x_g = M_go * M_ho^-1 * x_h
             * (M_ho^-1 = M_oh so x_g = M_go * M_oh * x_h)
             */

            btTransform invOriginToHairModel = btTransform::getIdentity();
            // Simplification: Always translation matrix, no invert needed
            invOriginToHairModel.getOrigin() -= util::convert(hairElement.position);

            btTransform hairWorldTransform;
            hairElement.body->getMotionState()->getWorldTransform(hairWorldTransform);
            glm::mat4 globalFromHair(1.0f);
            (hairWorldTransform * invOriginToHairModel).getOpenGLMatrix(glm::value_ptr(globalFromHair));

            hairModelToGlobalMatrices[++i] = m_cam->getView() * globalFromHair;
        }

        glUniformMatrix4fv(shader->model_view, static_cast<GLsizei>(hair->m_elements.size() + 1), GL_FALSE, glm::value_ptr(hairModelToGlobalMatrices[0]));

        glUniformMatrix4fv(shader->projection, 1, GL_FALSE, glm::value_ptr(m_cam->getProjection()));

        renderMesh(hair->m_mesh);
    }
}

/**
 * drawing world models.
 */
void Render::renderRoom(const world::Room* room)
{
    if(!m_skipRoom && room->m_mesh)
    {
        glm::mat4 modelViewProjection = m_cam->getViewProjection() * room->m_modelMatrix;

        UnlitTintedShaderDescription *shader = m_shaderManager->getRoomShader(room->m_lightMode == 1, room->m_flags & 1);

        glm::vec4 tint = engine::engine_world.calculateWaterTint();
        glUseProgram(shader->program);

        glUniform4fv(shader->tint_mult, 1, glm::value_ptr(tint));
        glUniform1f(shader->current_tick, static_cast<GLfloat>(SDL_GetTicks()));
        glUniform1i(shader->sampler, 0);
        glUniformMatrix4fv(shader->model_view_projection, 1, false, glm::value_ptr(modelViewProjection));
        renderMesh(room->m_mesh);
    }

    if(!room->m_staticMeshes.empty())
    {
        glUseProgram(m_shaderManager->getStaticMeshShader()->program);
        for(const std::shared_ptr<world::StaticMesh>& sm : room->m_staticMeshes)
        {
            if(sm->was_rendered)
            {
                continue;
            }

            if(sm->hide && !m_drawDummyStatics)
            {
                continue;
            }

            glm::mat4 transform = m_cam->getViewProjection() * sm->transform;
            glUniformMatrix4fv(m_shaderManager->getStaticMeshShader()->model_view_projection, 1, false, glm::value_ptr(transform));

            glm::vec4 tint = sm->tint;

            //If this static mesh is in a water room
            if(room->m_flags & TR_ROOM_FLAG_WATER)
            {
                tint *= engine::engine_world.calculateWaterTint();
            }
            glUniform4fv(m_shaderManager->getStaticMeshShader()->tint_mult, 1, glm::value_ptr(tint));
            renderMesh(sm->mesh);
            sm->was_rendered = true;
        }
    }

    for(world::Object* object : room->m_objects)
    {
        world::Entity* ent = dynamic_cast<world::Entity*>(object);
        if(!ent || ent->m_wasRendered)
            continue;

        renderEntity(ent);
        ent->m_wasRendered = true;
    }
}

void Render::renderRoomSprites(const world::Room* room)
{
    if(!room->m_sprites.empty() && room->m_spriteBuffer)
    {
        SpriteShaderDescription *shader = m_shaderManager->getSpriteShader();
        glUseProgram(shader->program);
        glUniformMatrix4fv(shader->model_view, 1, GL_FALSE, glm::value_ptr(m_cam->getView()));
        glUniformMatrix4fv(shader->projection, 1, GL_FALSE, glm::value_ptr(m_cam->getProjection()));
        glUniform1i(shader->sampler, 0);

        room->m_spriteBuffer->data->bind();

        size_t offset = 0;
        for(uint32_t texture = 0; texture < room->m_spriteBuffer->num_texture_pages; texture++)
        {
            if(room->m_spriteBuffer->element_count_per_texture[texture] == 0)
            {
                continue;
            }

            glBindTexture(GL_TEXTURE_2D, m_world->textures[texture]);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(room->m_spriteBuffer->element_count_per_texture[texture]), GL_UNSIGNED_SHORT, reinterpret_cast<GLvoid *>(offset * sizeof(uint16_t)));
            offset += room->m_spriteBuffer->element_count_per_texture[texture];
        }
    }
}

/**
 * Add a room to the render list.
 * If the room is already listed - false is returned and the room is not added twice.
 */
bool Render::addRoom(world::Room* room)
{
    if(!room->m_active || !m_renderList.insert(room).second)
    {
        return false;
    }

    if(room->m_flags & TR_ROOM_FLAG_SKYBOX)
        m_drawSkybox = true;

    for(std::shared_ptr<world::StaticMesh> sm : room->m_staticMeshes)
    {
        sm->was_rendered = false;
        sm->was_rendered_lines = false;
    }

    for(world::Object* object : room->m_objects)
    {
        world::Entity* ent = dynamic_cast<world::Entity*>(object);
        if(!ent)
            continue;

        ent->m_wasRendered = false;
        ent->m_wasRenderedLines = false;
    }

    for(world::RoomSprite& sp : room->m_sprites)
    {
        sp.was_rendered = false;
    }

    return true;
}

void Render::cleanList()
{
    if(m_world->character)
    {
        m_world->character->m_wasRendered = false;
        m_world->character->m_wasRenderedLines = false;
    }

    m_drawSkybox = false;
    m_renderList.clear();
}

/**
 * Render all visible rooms
 */
void Render::drawList()
{
    if(!m_world)
    {
        return;
    }

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    renderSkyBox();

    if(m_world->character)
    {
        renderEntity(m_world->character.get());
        renderHair(m_world->character);
    }

    /*
     * room rendering
     */
    for(const world::Room* room : m_renderList)
    {
        renderRoom(room);
    }

    glDisable(GL_CULL_FACE);

    ///@FIXME: reduce number of gl state changes
    for(const world::Room* room : m_renderList)
    {
        renderRoomSprites(room);
    }

    /*
     * NOW render transparency polygons
     */
    render_dBSP.reset();
    /*First generate BSP from base room mesh - it has good for start splitter polygons*/
    for(const world::Room* room : m_renderList)
    {
        if(room->m_mesh && !room->m_mesh->m_transparencyPolygons.empty())
        {
            render_dBSP.addNewPolygonList(room->m_mesh->m_transparentPolygons, room->m_modelMatrix, *m_cam);
        }
    }

    for(const world::Room* room : m_renderList)
    {
        // Add transparency polygons from static meshes (if they exists)
        for(auto sm : room->m_staticMeshes)
        {
            if(!sm->mesh->m_transparentPolygons.empty())
            {
                render_dBSP.addNewPolygonList(sm->mesh->m_transparentPolygons, sm->transform, *m_cam);
            }
        }

        // Add transparency polygons from all entities (if they exists) // yes, entities may be animated and intersects with each others;
        for(world::Object* object : room->m_objects)
        {
            world::Entity* ent = dynamic_cast<world::Entity*>(object);
            if(!ent)
                continue;

            if(!ent->m_skeleton.getModel()->has_transparency || !ent->m_visible)
                continue;

            for(const world::animation::Bone& bone : ent->m_skeleton.getBones())
            {
                if(!bone.mesh->m_transparencyPolygons.empty())
                {
                    auto tr = ent->m_transform * bone.full_transform;
                    render_dBSP.addNewPolygonList(bone.mesh->m_transparentPolygons, tr, *m_cam);
                }
            }
        }
    }

    if((engine::engine_world.character != nullptr) && engine::engine_world.character->m_skeleton.getModel()->has_transparency)
    {
        world::Entity *ent = engine::engine_world.character.get();
        for(const world::animation::Bone& bone : ent->m_skeleton.getBones())
        {
            if(!bone.mesh->m_transparencyPolygons.empty())
            {
                auto tr = ent->m_transform * bone.full_transform;
                render_dBSP.addNewPolygonList(bone.mesh->m_transparentPolygons, tr, *m_cam);
            }
        }
    }

    if(!render_dBSP.root()->polygons_front.empty())
    {
        UnlitTintedShaderDescription *shader = m_shaderManager->getRoomShader(false, false);
        glUseProgram(shader->program);
        glUniform1i(shader->sampler, 0);
        glUniformMatrix4fv(shader->model_view_projection, 1, false, glm::value_ptr(m_cam->getViewProjection()));
        glDepthMask(GL_FALSE);
        glDisable(GL_ALPHA_TEST);
        glEnable(GL_BLEND);
        renderBSPBackToFront(loader::BlendingMode::Opaque, render_dBSP.root(), shader);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

void Render::drawListDebugLines()
{
    if(!m_world || !(m_drawBoxes || m_drawRoomBoxes || m_drawPortals || m_drawAxis || m_drawNormals || m_drawColl))
    {
        return;
    }

    if(m_world->character)
    {
        debugDrawer.drawEntityDebugLines(m_world->character.get(), this);
    }

    /*
     * Render world debug information
     */
    if(m_drawNormals && m_world && m_world->sky_box)
    {
        glm::mat4 tr = glm::translate(glm::mat4(1.0f), m_cam->getPosition() + m_world->sky_box->animations.front().getInitialBoneKeyFrame().offset);
        tr *= glm::mat4_cast(m_world->sky_box->animations.front().getInitialBoneKeyFrame().qrotate);
        debugDrawer.drawMeshDebugLines(m_world->sky_box->meshes.front().mesh_base, tr, this);
    }

    for(const world::Room* room : m_renderList)
    {
        debugDrawer.drawRoomDebugLines(room, this);
    }

    if(m_drawColl)
    {
        engine::bt_engine_dynamicsWorld->debugDrawWorld();
    }

    if(!debugDrawer.IsEmpty())
    {
        UnlitShaderDescription *shader = m_shaderManager->getDebugLineShader();
        glUseProgram(shader->program);
        glUniform1i(shader->sampler, 0);
        glUniformMatrix4fv(shader->model_view_projection, 1, false, glm::value_ptr(m_cam->getViewProjection()));
        glBindTexture(GL_TEXTURE_2D, engine::engine_world.textures.back());
        glPointSize(6.0f);
        glLineWidth(3.0f);
        debugDrawer.render();
    }
}

struct PortalPath
{
    std::vector<const world::Portal*> portals;

    bool checkVisibility(const world::Portal* portal, const glm::vec3& cameraPosition, const world::core::Frustum& frustum)
    {
        if(!portal->destination || !portal->destination->m_active)
            return false; // no relevant destination

        if(glm::dot(portal->normal, portal->center - cameraPosition) >= 0)
            return false; // wrong orientation

        if(!frustum.isVisible(*portal))
            return false; // not in frustum

        if(portals.empty())
        {
            // no need to check the path
            portals.emplace_back(portal);
            return true;
        }

        // Now the heavy work: trace the portal path and test if we can see the target portal through any other portal.
        for(const world::Portal* testPortal : portals)
        {
            if(!testIntersection(cameraPosition, *portal, *testPortal))
                return false;
        }

        portals.emplace_back(portal);
        return true;
    }

    world::Room* getLastDestinationRoom() const
    {
        BOOST_ASSERT(!portals.empty());
        return portals.back()->destination;
    }

    const world::Portal* getLastPortal() const
    {
        BOOST_ASSERT(!portals.empty());
        return portals.back();
    }

private:
    static bool testIntersection(const glm::vec3& camPos, const world::Portal& a, const world::Portal& b)
    {
        // test if the ray from the camera to a's vertices crosses b's triangles
        for(const glm::vec3& v : a.vertices)
        {
            if( util::intersectRayTriangle(camPos, v-camPos, b.vertices[0], b.vertices[1], b.vertices[2]) )
                return true;
            if( util::intersectRayTriangle(camPos, v-camPos, b.vertices[2], b.vertices[3], b.vertices[0]) )
                return true;
        }

        // test if the ray from the camera to b's vertices crosses a's triangles
        for(const glm::vec3& v : b.vertices)
        {
            if( util::intersectRayTriangle(camPos, v-camPos, a.vertices[0], a.vertices[1], a.vertices[2]) )
                return true;
            if( util::intersectRayTriangle(camPos, v-camPos, a.vertices[2], a.vertices[3], a.vertices[0]) )
                return true;
        }

        return false;
    }
};

void Render::processRoom(world::Room* room)
{
    // Breadth-first queue
    std::queue<PortalPath> toVisit;

    addRoom(room);
    // always process direct neighbours
    for(const world::Portal& portal : room->m_portals)
    {
        PortalPath path;
        if(!path.checkVisibility(&portal, m_cam->getPosition(), m_cam->getFrustum()))
            continue;

        addRoom(portal.destination);

        toVisit.emplace(std::move(path));
    }

    // Avoid infinite loops
    std::set<const world::Portal*> visited;
    while(!toVisit.empty())
    {
        const PortalPath currentPath = std::move(toVisit.front());
        toVisit.pop();

        if(!visited.insert(currentPath.getLastPortal()).second)
            continue; // already tested

        world::Room* room = currentPath.getLastDestinationRoom();
        bool roomIsVisible = false;
        for(const world::Portal& srcPortal : room->m_portals)
        {
            PortalPath newPath = currentPath;
            if(!newPath.checkVisibility(&srcPortal, m_cam->getPosition(), m_cam->getFrustum()))
                continue;

            roomIsVisible = true;
            toVisit.emplace(std::move(newPath));
        }
        if(roomIsVisible)
            addRoom(room);
    }
}

// Renderer list generation by current world and camera

void Render::genWorldList()
{
    if(m_world == nullptr)
    {
        return;
    }

    cleanList();                              // clear old render list
    debugDrawer.reset();
    //cam->frustum->next = NULL;

    // find room that contains camera
    world::Room* curr_room = Room_FindPosCogerrence(m_cam->getPosition(), m_cam->getCurrentRoom());

    m_cam->setCurrentRoom( curr_room );
    if(curr_room != nullptr)                  // camera located in some room
    {
        processRoom(curr_room);
        return;
    }

    if(engine::control_states.noclip)  // camera is out of all rooms AND noclip is on
    {
        for(auto r : m_world->rooms)
        {
            if(m_cam->getFrustum().isVisible(r->m_boundingBox, *m_cam))
            {
                addRoom(r.get());
            }
        }
    }

#if 0
    // Enable/disable collision shapes
    for(std::shared_ptr<world::Room> room : m_world->rooms)
    {
        if(m_renderList.find(room.get()) == m_renderList.end())
            room->disable();
        else
            room->enable();
    }
#endif
}

// Coupling renderer and the world.

void Render::setWorld(world::World *world)
{
    resetWorld();

    m_world = world;
    m_drawSkybox = false;
    m_renderList.clear();

    m_cam = &engine::engine_camera;
    engine::engine_camera.setCurrentRoom( nullptr );
}


// Render debug primitives.

RenderDebugDrawer::RenderDebugDrawer()
{
}

RenderDebugDrawer::~RenderDebugDrawer()
{
}

void RenderDebugDrawer::reset()
{
    m_buffer.clear();
}

void RenderDebugDrawer::addLine(const glm::vec3& start, const glm::vec3& end)
{
    addLine(start, m_color, end, m_color);
}

void RenderDebugDrawer::addLine(const glm::vec3& start, const glm::vec3& startColor, const glm::vec3& end, const glm::vec3& endColor)
{
    m_buffer.emplace_back(start);
    m_buffer.emplace_back(startColor);
    m_buffer.emplace_back(end);
    m_buffer.emplace_back(endColor);
}

void RenderDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3 &color)
{
    addLine(util::convert(from), util::convert(color), util::convert(to), util::convert(color));
}

void RenderDebugDrawer::setDebugMode(int debugMode)
{
    m_debugMode = debugMode;
}

void RenderDebugDrawer::draw3dText(const btVector3& /*location*/, const char* /*textString*/)
{
    //glRasterPos3f(location.x(),  location.y(),  location.z());
    //BMF_DrawString(BMF_GetFont(BMF_kHelvetica10),textString);
}

void RenderDebugDrawer::reportErrorWarning(const char* warningString)
{
    Console::instance().addLine(warningString, gui::FontStyle::ConsoleWarning);
}

void RenderDebugDrawer::drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar distance, int /*lifeTime*/, const btVector3& color)
{
    drawLine(pointOnB, pointOnB + normalOnB * distance, color);
}

void RenderDebugDrawer::render()
{
    if(!m_buffer.empty())
    {
        if(m_glbuffer == 0)
        {
            glGenBuffers(1, &m_glbuffer);
            VertexArrayAttribute attribs[] = {
                VertexArrayAttribute(UnlitShaderDescription::Position, 3, GL_FLOAT, false, m_glbuffer, sizeof(glm::vec3)*2, 0),
                VertexArrayAttribute(UnlitShaderDescription::Color,    3, GL_FLOAT, false, m_glbuffer, sizeof(glm::vec3)*2, sizeof(glm::vec3))
            };
            m_vertexArray.reset(new VertexArray(0, 2, attribs));
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_glbuffer);
        glBufferData(GL_ARRAY_BUFFER, m_buffer.size() * sizeof(m_buffer[0]), nullptr, GL_STREAM_DRAW);

        glm::vec3* data = static_cast<glm::vec3*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
        std::copy(m_buffer.begin(), m_buffer.end(), data);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        m_vertexArray->bind();
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_buffer.size() / 2));
    }

    m_color = {0,0,0};
    m_buffer.clear();
}

void RenderDebugDrawer::drawAxis(glm::float_t r, const glm::mat4 &transform)
{
    glm::vec3 origin{transform[3]};

    glm::vec4 v = transform[0] * r;
    v += transform[3];
    m_buffer.push_back(origin);
    m_buffer.push_back({1.0, 0.0, 0.0});
    m_buffer.push_back({v.x, v.y, v.z});
    m_buffer.push_back({1.0, 0.0, 0.0});

    v = transform[1] * r;
    v += transform[3];
    m_buffer.push_back(origin);
    m_buffer.push_back({0.0, 1.0, 0.0});
    m_buffer.push_back({v.x, v.y, v.z});
    m_buffer.push_back({0.0, 1.0, 0.0});

    v = transform[2] * r;
    v += transform[3];
    m_buffer.push_back(origin);
    m_buffer.push_back({0.0, 0.0, 1.0});
    m_buffer.push_back({v.x, v.y, v.z});
    m_buffer.push_back({0.0, 0.0, 1.0});
}

void RenderDebugDrawer::drawPortal(const world::Portal& p)
{
    addLine(p.vertices[0], p.vertices[1]);
    addLine(p.vertices[1], p.vertices[2]);
    addLine(p.vertices[2], p.vertices[3]);
    addLine(p.vertices[3], p.vertices[0]);
    addLine(p.vertices[1], p.vertices[3]);
}

void RenderDebugDrawer::drawBBox(const world::core::BoundingBox& boundingBox, const glm::mat4 *transform)
{
    m_obb.rebuild(boundingBox);
    m_obb.transform = transform;
    m_obb.doTransform();
    drawOBB(m_obb);
}

void RenderDebugDrawer::drawOBB(const world::core::OrientedBoundingBox& obb)
{
    const world::core::Polygon *p = obb.polygons.data();
    addLine(p->vertices[0].position, (p + 1)->vertices[0].position);
    addLine(p->vertices[1].position, (p + 1)->vertices[3].position);
    addLine(p->vertices[2].position, (p + 1)->vertices[2].position);
    addLine(p->vertices[3].position, (p + 1)->vertices[1].position);

    for(int i = 0; i < 2; i++, p++)
    {
        for(size_t j = 0; j < p->vertices.size() - 1; j++)
        {
            addLine(p->vertices[j].position, p->vertices[j + 1].position);
        }
        addLine(p->vertices.back().position, p->vertices.front().position);
    }
}

void RenderDebugDrawer::drawMeshDebugLines(const std::shared_ptr<world::core::BaseMesh> &mesh, const glm::mat4& transform, Render* render)
{
    if(!render->m_drawNormals)
        return;

    setColor(0.8f, 0.0f, 0.9f);
    for(const world::core::Vertex& v : mesh->m_vertices)
    {
        glm::vec4 tv = transform * glm::vec4(v.position, 1.0f);
        m_buffer.push_back({tv.x, tv.y, tv.z});
        m_buffer.emplace_back(m_color);
        tv += glm::vec4(glm::mat3(transform) * v.normal * 128.0f, 0);
        m_buffer.push_back({tv.x, tv.y, tv.z});
        m_buffer.emplace_back(m_color);
    }
}

void RenderDebugDrawer::drawSkeletalModelDebugLines(const world::animation::Skeleton& skeleton, const glm::mat4& transform, Render *render)
{
    if(!render->m_drawNormals)
        return;

    for(const world::animation::Bone& bone : skeleton.getBones())
        drawMeshDebugLines(bone.mesh, transform * bone.full_transform, render);
}

void RenderDebugDrawer::drawEntityDebugLines(world::Entity* entity, Render* render)
{
    if(entity->m_wasRenderedLines || !(render->m_drawAxis || render->m_drawNormals || render->m_drawBoxes) || !entity->m_visible)
    {
        return;
    }

    if(render->m_drawBoxes)
    {
        debugDrawer.setColor(0.0, 0.0, 1.0);
        debugDrawer.drawOBB(entity->m_obb);
    }

    if(render->m_drawAxis)
    {
        // If this happens, the lines after this will get drawn with random colors. I don't care.
        debugDrawer.drawAxis(1000.0, entity->m_transform);
    }

    if(entity->m_skeleton.getModel() && !entity->m_skeleton.getModel()->animations.empty())
    {
        debugDrawer.drawSkeletalModelDebugLines(entity->m_skeleton, entity->m_transform, render);
    }

    entity->m_wasRenderedLines = true;
}

void RenderDebugDrawer::drawSectorDebugLines(world::RoomSector *rs)
{
    world::core::BoundingBox bb;
    bb.min = { rs->position[0] - world::MeteringSectorSize / 2.0f, rs->position[1] - world::MeteringSectorSize / 2.0f, static_cast<glm::float_t>(rs->floor) };
    bb.max = { rs->position[0] + world::MeteringSectorSize / 2.0f, rs->position[1] + world::MeteringSectorSize / 2.0f, static_cast<glm::float_t>(rs->ceiling) };

    drawBBox(bb, nullptr);
}

void RenderDebugDrawer::drawRoomDebugLines(const world::Room* room, Render* render)
{
    if(render->m_drawRoomBoxes)
    {
        debugDrawer.setColor(0.0, 0.1f, 0.9f);
        debugDrawer.drawBBox(room->m_boundingBox, nullptr);
        /*for(uint32_t s=0;s<room->sectors_count;s++)
        {
            drawSectorDebugLines(room->sectors + s);
        }*/
    }

    if(render->m_drawPortals)
    {
        debugDrawer.setColor(1.0, 1.0, 0.0);
        for(const auto& p : room->m_portals)
            debugDrawer.drawPortal(p);

        debugDrawer.setColor(1.0, 1.0, 0.5);
        for(const auto& p : room->m_portals)
            debugDrawer.addLine(p.center, p.center + p.normal*128.0f);
    }

    if(!render->m_skipRoom && (room->m_mesh != nullptr))
    {
        debugDrawer.drawMeshDebugLines(room->m_mesh, room->m_modelMatrix, render);
    }

    for(auto sm : room->m_staticMeshes)
    {
        if(sm->was_rendered_lines || (sm->hide && !render->m_drawDummyStatics))
        {
            continue;
        }

        if(render->m_drawBoxes)
        {
            debugDrawer.setColor(0.0f, 1.0f, 0.1f);
            debugDrawer.drawOBB(sm->obb);
        }

        if(render->m_drawAxis)
        {
            debugDrawer.drawAxis(1000.0, sm->transform);
        }

        debugDrawer.drawMeshDebugLines(sm->mesh, sm->transform, render);

        sm->was_rendered_lines = true;
    }

    for(world::Object* object : room->m_objects)
    {
        world::Entity* ent = dynamic_cast<world::Entity*>(object);
        if(!ent)
            continue;

        if(ent->m_wasRenderedLines)
            continue;

        debugDrawer.drawEntityDebugLines(ent, render);
        ent->m_wasRenderedLines = true;
    }
}

/**
 * The base function, that draws one item by them id. Items may be animated.
 * This time for correct time calculation that function must be called every frame.
 * @param item_id - the base item id;
 * @param size - the item size on the screen;
 * @param str - item description - shows near / under item model;
 */
void renderItem(world::animation::Skeleton *bf, glm::float_t size, const glm::mat4& mvMatrix, const glm::mat4& guiProjectionMatrix)
{
    const LitShaderDescription *shader = renderer.shaderManager()->getEntityShader(0, false);
    glUseProgram(shader->program);
    glUniform1i(shader->number_of_lights, 0);
    glUniform4f(shader->light_ambient, 1.f, 1.f, 1.f, 1.f);

    if(size != 0.0)
    {
        auto bb = bf->getBoundingBox().getDiameter();
        size /= glm::max(glm::max(bb[0], bb[1]), bb[2]);
        size *= 0.8f;

        glm::mat4 scaledMatrix = glm::mat4(1.0f);
        if(size < 1.0)          // only reduce items size...
        {
            scaledMatrix = glm::scale(scaledMatrix, { size, size, size });
        }
        glm::mat4 scaledMvMatrix(mvMatrix * scaledMatrix);
        glm::mat4 mvpMatrix = guiProjectionMatrix * scaledMvMatrix;

        // Render with scaled model view projection matrix
        // Use original modelview matrix, as that is used for normals whose size shouldn't change.
        renderer.renderSkeletalModel(shader, bf, mvMatrix, mvpMatrix/*, guiProjectionMatrix*/);
    }
    else
    {
        glm::mat4 mvpMatrix = guiProjectionMatrix * mvMatrix;
        renderer.renderSkeletalModel(shader, bf, mvMatrix, mvpMatrix/*, guiProjectionMatrix*/);
    }
}

namespace
{
GLuint crosshairBuffer = 0;
VertexArray *crosshairArray = nullptr;
}

void fillCrosshairBuffer()
{
    if(!crosshairBuffer)
        glGenBuffers(1, &crosshairBuffer);

    struct BufferEntry
    {
        glm::vec2 position;
        uint8_t color[4];
    };

    BufferEntry crosshair_buf[4] = {
        {{static_cast<glm::float_t>(engine::screen_info.w / 2.0f - 5.f), (static_cast<glm::float_t>(engine::screen_info.h) / 2.0f)}, {255, 0, 0, 255}},
        {{static_cast<glm::float_t>(engine::screen_info.w / 2.0f + 5.f), (static_cast<glm::float_t>(engine::screen_info.h) / 2.0f)}, {255, 0, 0, 255}},
        {{static_cast<glm::float_t>(engine::screen_info.w / 2.0f), (static_cast<glm::float_t>(engine::screen_info.h) / 2.0f - 5.f)}, {255, 0, 0, 255}},
        {{static_cast<glm::float_t>(engine::screen_info.w / 2.0f), (static_cast<glm::float_t>(engine::screen_info.h) / 2.0f + 5.f)}, {255, 0, 0, 255}}
    };

    glBindBuffer(GL_ARRAY_BUFFER, crosshairBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_buf), crosshair_buf, GL_STATIC_DRAW);

    VertexArrayAttribute attribs[] = {
        VertexArrayAttribute(GuiShaderDescription::position, 2, GL_FLOAT, false, crosshairBuffer, sizeof(BufferEntry), offsetof(BufferEntry, position)),
        VertexArrayAttribute(GuiShaderDescription::color, 4, GL_UNSIGNED_BYTE, true, crosshairBuffer, sizeof(BufferEntry), offsetof(BufferEntry, color))
    };
    crosshairArray = new VertexArray(0, 2, attribs);
}

void drawCrosshair()
{
    GuiShaderDescription *shader = renderer.shaderManager()->getGuiShader(false);

    glUseProgram(shader->program);
    glm::vec2 factor = {
        2.0f / engine::screen_info.w,
        2.0f / engine::screen_info.h
    };
    glUniform2fv(shader->factor, 1, glm::value_ptr(factor));
    glm::vec2 offset = { -1.f, -1.f };
    glUniform2fv(shader->offset, 1, glm::value_ptr(offset));

    crosshairArray->bind();

    glDrawArrays(GL_LINES, 0, 4);
}

} // namespace render

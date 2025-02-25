#pragma once

#include "audioengine.h"
#include "cameracontroller.h"
#include "floordata/floordata.h"
#include "inventory.h"
#include "items_tr1.h"
#include "loader/file/animationid.h"
#include "loader/file/item.h"
#include "render/scene/ScreenOverlay.h"
#include "util/cimgwrapper.h"

#include <boost/filesystem/path.hpp>
#include <memory>

namespace hid
{
class InputHandler;
}

namespace loader
{
namespace file
{
namespace level
{
class Level;
}

struct TextureKey;
struct Room;
struct Sector;
struct Mesh;
struct SkeletalModelType;
struct Box;
struct StaticMesh;
struct SpriteSequence;
struct AnimFrame;
struct Animation;
struct CinematicFrame;
} // namespace file
} // namespace loader

namespace render
{
namespace gl
{
class Font;
}

class RenderPipeline;
} // namespace render

namespace engine
{
namespace items
{
class ItemNode;

class PickupItem;
} // namespace items

class Particle;

class Engine
{
private:
    std::shared_ptr<loader::file::level::Level> m_level;
    std::unique_ptr<CameraController> m_cameraController = nullptr;

    std::unique_ptr<AudioEngine> m_audioEngine;

    core::Frame m_effectTimer = 0_frame;
    boost::optional<size_t> m_activeEffect{};

    std::map<uint16_t, gsl::not_null<std::shared_ptr<items::ItemNode>>> m_itemNodes;

    std::set<gsl::not_null<std::shared_ptr<items::ItemNode>>> m_dynamicItems;

    std::set<items::ItemNode*> m_scheduledDeletions;

    std::vector<gsl::not_null<std::shared_ptr<render::scene::Model>>> m_models;

    int m_uvAnimTime{0};

    std::shared_ptr<render::scene::ShaderProgram> m_lightningShader;

    sol::state m_scriptEngine;

    std::shared_ptr<LaraNode> m_lara = nullptr;

    std::shared_ptr<render::TextureAnimator> m_textureAnimator;

    std::unique_ptr<hid::InputHandler> m_inputHandler;

    bool m_roomsAreSwapped = false;

    std::vector<gsl::not_null<std::shared_ptr<Particle>>> m_particles;

    // list of meshes and models, resolved through m_meshIndices
    std::vector<gsl::not_null<std::shared_ptr<render::scene::Model>>> m_modelsDirect;
    std::vector<gsl::not_null<const loader::file::Mesh*>> m_meshesDirect;

    std::shared_ptr<render::scene::Material> m_spriteMaterial{nullptr};
    std::shared_ptr<render::scene::Material> m_portalMaterial{nullptr};

    std::shared_ptr<render::RenderPipeline> m_renderPipeline;
    std::shared_ptr<render::scene::ScreenOverlay> screenOverlay;
    std::unique_ptr<render::scene::Renderer> m_renderer;
    std::unique_ptr<render::scene::Window> m_window;
    sol::table levelInfo;

    const util::CImgWrapper splashImage;
    util::CImgWrapper splashImageScaled;
    std::shared_ptr<render::gl::Font> abibasFont;

    bool m_levelFinished = false;

    Inventory m_inventory;

    struct PositionalEmitter final : public audio::Emitter
    {
        glm::vec3 position;

        PositionalEmitter(const glm::vec3& position, const gsl::not_null<audio::SoundEngine*>& engine)
            : Emitter{engine}
            , position{position}
        {
        }

        glm::vec3 getPosition() const override
        {
            return position;
        }
    };

    std::vector<PositionalEmitter> m_positionalEmitters;
    core::Health m_drawnHealth = core::LaraHealth;
    core::Frame m_healthBarTimeout = -40_frame;

public:
    explicit Engine(bool fullscreen = false, const render::scene::Dimension2<int>& resolution = {1280, 800});

    ~Engine();

    const hid::InputHandler& getInputHandler() const
    {
        return *m_inputHandler;
    }

    bool roomsAreSwapped() const
    {
        return m_roomsAreSwapped;
    }

    LaraNode& getLara()
    {
        return *m_lara;
    }

    const LaraNode& getLara() const
    {
        return *m_lara;
    }

    auto& getParticles()
    {
        return m_particles;
    }

    auto& getItemNodes()
    {
        return m_itemNodes;
    }

    const auto& getItemNodes() const
    {
        return m_itemNodes;
    }

    const auto& getDynamicItems() const
    {
        return m_dynamicItems;
    }

    CameraController& getCameraController()
    {
        return *m_cameraController;
    }

    const CameraController& getCameraController() const
    {
        return *m_cameraController;
    }

    const auto& getSpriteMaterial() const
    {
        return m_spriteMaterial;
    }

    auto& getSoundEngine()
    {
        return m_audioEngine->m_soundEngine;
    }

    auto& getScriptEngine()
    {
        return m_scriptEngine;
    }

    const auto& getScriptEngine() const
    {
        return m_scriptEngine;
    }

    auto& getAudioEngine()
    {
        return *m_audioEngine;
    }

    const auto& getAudioEngine() const
    {
        return *m_audioEngine;
    }

    auto& getInventory()
    {
        return m_inventory;
    }

    const auto& getInventory() const
    {
        return m_inventory;
    }

    bool hasLevel() const
    {
        return m_level != nullptr;
    }

    void finishLevel()
    {
        m_levelFinished = true;
    }

    void run();

    std::map<loader::file::TextureKey, gsl::not_null<std::shared_ptr<render::scene::Material>>>
        createMaterials(const gsl::not_null<std::shared_ptr<render::scene::ShaderProgram>>& shader);

    std::shared_ptr<LaraNode> createItems();

    void loadSceneData(bool linearTextureInterpolation);

    const std::unique_ptr<loader::file::SkeletalModelType>& findAnimatedModelForType(core::TypeId type) const;

    template<typename T>
    std::shared_ptr<T> createItem(const core::TypeId type,
                                  const gsl::not_null<const loader::file::Room*>& room,
                                  const core::Angle& angle,
                                  const core::TRVec& position,
                                  const uint16_t activationState)
    {
        const auto& model = findAnimatedModelForType(type);
        if(model == nullptr)
            return nullptr;

        loader::file::Item item;
        item.type = type;
        item.room = uint16_t(-1);
        item.position = position;
        item.rotation = angle;
        item.darkness = 0;
        item.activationState = activationState;

        auto node = std::make_shared<T>(this, room, item, *model);

        m_dynamicItems.emplace(node);
        addChild(room->node, node->getNode());

        return node;
    }

    std::shared_ptr<items::PickupItem> createPickup(core::TypeId type,
                                                    const gsl::not_null<const loader::file::Room*>& room,
                                                    const core::TRVec& position);

    std::tuple<int8_t, int8_t> getFloorSlantInfo(gsl::not_null<const loader::file::Sector*> sector,
                                                 const core::TRVec& position) const;

    std::shared_ptr<items::ItemNode> getItem(uint16_t id) const;

    void drawBars(const gsl::not_null<std::shared_ptr<render::gl::Image<render::gl::SRGBA8>>>& image);

    void useAlternativeLaraAppearance(bool withHead = false);

    const gsl::not_null<std::shared_ptr<render::scene::Model>>& getModel(const size_t idx) const
    {
        return m_models.at(idx);
    }

    void scheduleDeletion(items::ItemNode* item)
    {
        m_scheduledDeletions.insert(item);
    }

    void applyScheduledDeletions()
    {
        if(m_scheduledDeletions.empty())
            return;

        for(const auto& del : m_scheduledDeletions)
        {
            auto it = std::find_if(m_dynamicItems.begin(),
                                   m_dynamicItems.end(),
                                   [del](const std::shared_ptr<items::ItemNode>& i) { return i.get() == del; });
            if(it == m_dynamicItems.end())
                continue;
            m_dynamicItems.erase(it);
        }

        m_scheduledDeletions.clear();
    }

    void turn180Effect(items::ItemNode& node);

    void dinoStompEffect(items::ItemNode& node);

    void laraNormalEffect();

    void laraBubblesEffect(items::ItemNode& node);

    void finishLevelEffect();

    void earthquakeEffect();

    void floodEffect();

    void chandelierEffect();

    void raisingBlockEffect();

    void stairsToSlopeEffect();

    void sandEffect();

    void explosionEffect();

    void laraHandsFreeEffect();

    void flipMapEffect();

    void unholsterRightGunEffect(items::ItemNode& node);

    void chainBlockEffect();

    void flickerEffect();

    void swapAllRooms();

    void setGlobalEffect(size_t fx)
    {
        m_activeEffect = fx;
        m_effectTimer = 0_frame;
    }

    void doGlobalEffect();

    void runEffect(const size_t id, items::ItemNode* node)
    {
        switch(id)
        {
        case 0: Expects(node != nullptr); return turn180Effect(*node);
        case 1: Expects(node != nullptr); return dinoStompEffect(*node);
        case 2: return laraNormalEffect();
        case 3: Expects(node != nullptr); return laraBubblesEffect(*node);
        case 4: return finishLevelEffect();
        case 5: return earthquakeEffect();
        case 6: return floodEffect();
        case 7: return chandelierEffect();
        case 8: return raisingBlockEffect();
        case 9: return stairsToSlopeEffect();
        case 10: return sandEffect();
        case 11: return explosionEffect();
        case 12: return laraHandsFreeEffect();
        case 13: return flipMapEffect();
        case 14: Expects(node != nullptr); return unholsterRightGunEffect(*node);
        case 15: return chainBlockEffect();
        case 16: return flickerEffect();
        default: BOOST_LOG_TRIVIAL(warning) << "Unhandled effect: " << id;
        }
    }

    void swapWithAlternate(loader::file::Room& orig, loader::file::Room& alternate);

    void animateUV();

    YAML::Node save() const;

    void load(const YAML::Node& node);

    boost::optional<size_t> indexOfModel(const std::shared_ptr<render::scene::Renderable>& m) const
    {
        if(m == nullptr)
            return boost::none;

        for(size_t i = 0; i < m_models.size(); ++i)
            if(m_models[i].get() == m)
                return i;

        return boost::none;
    }

    std::array<floordata::ActivationState, 10> mapFlipActivationStates;

    items::ItemNode* m_pierre = nullptr;

    const std::vector<loader::file::Box>& getBoxes() const;

    const std::vector<loader::file::Room>& getRooms() const;

    const loader::file::StaticMesh* findStaticMeshById(core::StaticMeshId meshId) const;

    const std::unique_ptr<loader::file::SpriteSequence>& findSpriteSequenceForType(core::TypeId type) const;

    bool isValid(const loader::file::AnimFrame* frame) const;

    const loader::file::Animation& getAnimation(loader::file::AnimationId id) const;

    const std::vector<loader::file::Animation>& getAnimations() const;

    const std::vector<loader::file::CinematicFrame>& getCinematicFrames() const;

    const std::vector<loader::file::Camera>& getCameras() const;

    const std::vector<int16_t>& getAnimCommands() const;

    const std::vector<uint16_t>& getOverlaps() const;

    void update(bool godMode);

    static void drawText(const gsl::not_null<std::shared_ptr<render::gl::Font>>& font,
                         int x,
                         const int y,
                         const std::string& txt,
                         const render::gl::SRGBA8& col = {255, 255, 255, 255});

    void drawDebugInfo(const gsl::not_null<std::shared_ptr<render::gl::Font>>& font, float fps);

    void scaleSplashImage();

    void drawLoadingScreen(const std::string& state);
    ;

    const std::vector<int16_t>& getPoseFrames() const;
};
} // namespace engine

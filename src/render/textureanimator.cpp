#include "textureanimator.h"

#include "loader/file/datatypes.h"
#include "loader/file/texture.h"
#include "util/cimgwrapper.h"

#include <memory>

namespace render
{
namespace
{
/**
 * @brief A Binary Space Partition Tree for 2D space.
 */
struct BSPTree
{
    std::unique_ptr<BSPTree> left;
    std::unique_ptr<BSPTree> right;

    //! If @c true, denotes that there is no more free space in this node or its children.
    //! @note This is a pure caching mechanism to avoid unnecessary recursion.
    bool isFilled = false;

    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    BSPTree() = default;

    BSPTree(const int32_t x, const int32_t y, const int32_t w, const int32_t h)
        : x{x}
        , y{y}
        , width{w}
        , height{h}
    {
        Expects(x >= 0);
        Expects(y >= 0);
        Expects(w > 0);
        Expects(h > 0);
    }

    bool isSplit() const
    {
        return left != nullptr && right != nullptr;
    }

    /**
     * @brief Split this node along its Y axis (X is split).
     * @param splitLocation Local X coordinate of the split point
     */
    void splitX(const int32_t splitLocation)
    {
        Expects(splitLocation < width);
        left = std::make_unique<BSPTree>(x, y, splitLocation, height);
        right = std::make_unique<BSPTree>(x + splitLocation, y, width - splitLocation, height);
    }

    /**
     * @brief Split this node along its X axis (Y is split).
     * @param splitLocation Local Y coordinate of the split point
     */
    void splitY(const int32_t splitLocation)
    {
        Expects(splitLocation < height);
        left = std::make_unique<BSPTree>(x, y, width, splitLocation);
        right = std::make_unique<BSPTree>(x, y + splitLocation, width, height - splitLocation);
    }

    bool fits(const int32_t w, const int32_t h) const noexcept
    {
        Expects(w > 0);
        Expects(h > 0);
        return !isFilled && w <= width && h <= height;
    }

    /**
     * @brief Find a free space in this node or its children
     */
    boost::optional<glm::vec2> tryInsert(const int scale, const glm::vec2& uv)
    {
        const auto tmp = tryInsert(gsl::narrow_cast<int32_t>(scale * uv.x), gsl::narrow_cast<int32_t>(scale * uv.y));
        if(!tmp.is_initialized())
            return boost::none;

        return glm::vec2{tmp->x / float(scale), tmp->y / float(scale)};
    }

    boost::optional<glm::ivec2> tryInsert(const int32_t width, const int32_t height)
    {
        // Could this possibly fit?
        if(!fits(width, height))
            return boost::none;

        if(isSplit())
        {
            // This node is already split => Recurse!
            boost::optional<glm::ivec2> found{};
            if(width <= left->width && height <= left->height)
            {
                found = left->tryInsert(width, height);
            }

            if(!found.is_initialized() && width <= right->width && height <= right->height)
            {
                found = right->tryInsert(width, height);
            }

            // If both children are filled, mark this node as filled and discard the children.
            if(left->isFilled && right->isFilled)
            {
                isFilled = true;
                left.reset();
                right.reset();
            }

            return found;
        }

        // We may split this node
        if(this->height == height && this->width == width)
        {
            // Perfect match
            isFilled = true;
            return glm::ivec2{x, y};
        }
        else if(this->height == height)
        {
            // Split horizontally
            splitX(width);

            // height already fits, width fits too now, so this is the result
            left->isFilled = true;
            return glm::ivec2{x, y};
        }
        else
        {
            // In case of doubt do a vertical split
            splitY(height);

            // Recurse, because the width may not match
            return left->tryInsert(width, height);
        }
    }
};

class TextureAtlas
{
    struct TextureSizeComparator
    {
        const TextureAtlas* const context;

        explicit TextureSizeComparator(const TextureAtlas* context_)
            : context{context_}
        {
        }

        bool operator()(size_t index1, size_t index2) const
        {
            const Mapping& m1 = context->m_mappings[index1];
            const Mapping& m2 = context->m_mappings[index2];

            // First order by height.
            auto tmp = (m1.uvMax.y - m1.uvMin.y) - (m2.uvMax.y - m2.uvMin.y);
            if(tmp > 0)
                return true;
            else if(tmp < 0)
                return false;

            // Then order by width
            tmp = (m1.uvMax.x - m1.uvMin.x) - (m2.uvMax.x - m2.uvMin.x);
            if(tmp > 0)
                return true;
            else if(tmp < 0)
                return false;

            // If they have the same height and width then their order does not matter.
            return false;
        }
    };

    struct Mapping
    {
        glm::vec2 uvMin;
        glm::vec2 uvMax;

        loader::file::TextureKey srcTexture;

        glm::vec2 newUvMin;
    };

    const int32_t m_resultPageSize;
    std::map<core::TextureTileId, size_t> m_mappingByTile{};
    std::vector<Mapping> m_mappings{};

public:
    void layOutTextures(const std::vector<loader::file::DWordTexture>& textures)
    {
        // First step: Sort the canonical textures by size.
        std::vector<size_t> sortedIndices(m_mappings.size());
        for(size_t i = 0; i < m_mappings.size(); i++)
            sortedIndices[i] = i;

        std::sort(sortedIndices.begin(), sortedIndices.end(), TextureSizeComparator(this));

        // Find positions for the canonical textures
        BSPTree layout{0, 0, m_resultPageSize, m_resultPageSize};

        for(size_t texture = 0; texture < m_mappings.size(); texture++)
        {
            Mapping& mapping = m_mappings[sortedIndices[texture]];
            const auto mapped = layout.tryInsert(
                textures.at(mapping.srcTexture.tileAndFlag & loader::file::TextureIndexMask).image->getWidth(),
                mapping.uvMax - mapping.uvMin);
            if(!mapped.is_initialized())
            {
                BOOST_THROW_EXCEPTION(std::runtime_error("UV animation texture overflow"));
            }

            mapping.newUvMin = *mapped;
        }
    }

    explicit TextureAtlas(const int32_t pageSize)
        : m_resultPageSize{pageSize}
    {
        Expects(pageSize > 0);
    }

    ~TextureAtlas() = default;

    void insert(const std::vector<loader::file::TextureTile>& textureTiles, const core::TextureTileId tileId)
    {
        const auto& tile = textureTiles.at(tileId.get());
        // Determine the canonical texture for this texture.
        // Use only first three vertices to find min, max, because for triangles the last will be 0,0 with no other marker that this is a triangle. As long as all textures are axis-aligned rectangles, this will always return the right result anyway.
        glm::vec2 max{0, 0}, min{1.0f, 1.0f};
        for(const auto& uv : tile.uvCoordinates)
        {
            const auto gl = uv.toGl();
            max.x = std::max(max.x, gl.x);
            max.y = std::max(max.y, gl.y);
            min.x = std::min(min.x, gl.x);
            min.y = std::min(min.y, gl.y);
        }

        // See whether it already exists
        size_t mappingIdx = std::numeric_limits<size_t>::max();
        for(size_t i = 0; i < m_mappings.size(); i++)
        {
            if(m_mappings[i].srcTexture == tile.textureKey && m_mappings[i].uvMin == min && m_mappings[i].uvMax == max)
            {
                mappingIdx = i;
                break;
            }
        }

        // Create it if not.
        if(mappingIdx == std::numeric_limits<size_t>::max())
        {
            mappingIdx = m_mappings.size();
            m_mappings.emplace_back();

            Mapping& mapping = m_mappings.back();
            mapping.srcTexture = tile.textureKey;
            mapping.uvMin = min;
            mapping.uvMax = max;
        }

        m_mappingByTile.emplace(std::make_pair(tileId, mappingIdx));
    }

    void toTexture(std::vector<loader::file::DWordTexture>& textures,
                   std::vector<loader::file::TextureTile>& textureTiles,
                   bool linear) const
    {
        util::CImgWrapper img{m_resultPageSize};

        for(const auto& tileToMapping : m_mappingByTile)
        {
            auto& tile = textureTiles.at(tileToMapping.first.get());
            const auto& mapping = m_mappings.at(tileToMapping.second);

            const auto& glSrcImg = textures.at(mapping.srcTexture.tileAndFlag & loader::file::TextureIndexMask).image;
            util::CImgWrapper srcImg{reinterpret_cast<const uint8_t*>(glSrcImg->getData().data()),
                                     glSrcImg->getWidth(),
                                     glSrcImg->getHeight(),
                                     true};
            srcImg.crop(mapping.uvMin, mapping.uvMax);
            srcImg.resize(srcImg.width() * m_resultPageSize / glSrcImg->getWidth(),
                          srcImg.height() * m_resultPageSize / glSrcImg->getHeight());
            img.replace(static_cast<int>(mapping.newUvMin.x * m_resultPageSize),
                        static_cast<int>(mapping.newUvMin.y * m_resultPageSize),
                        srcImg);

            const auto s = static_cast<float>(m_resultPageSize)
                           / textures.at(tile.textureKey.tileAndFlag & 0x7fff).image->getWidth();

            tile.textureKey.tileAndFlag &= ~loader::file::TextureIndexMask;
            tile.textureKey.tileAndFlag |= gsl::narrow_cast<uint16_t>(textures.size());
            for(auto& uv : tile.uvCoordinates)
            {
                const auto d = uv.toGl() - mapping.uvMin;
                uv = d * s + mapping.newUvMin;
            }
        }

        loader::file::DWordTexture texture;
        texture.texture = std::make_shared<render::gl::Texture2D<render::gl::SRGBA8>>("animated texture tiles");
        texture.texture->set(::gl::TextureMinFilter::NearestMipmapLinear).generateMipmap();
        if(!linear)
        {
            texture.texture->set(::gl::TextureMagFilter::Nearest);
        }
        else
        {
            texture.texture->set(::gl::TextureMagFilter::Linear);
        }
        img.interleave();
        texture.image = std::make_shared<render::gl::Image<render::gl::SRGBA8>>(
            img.width(), img.height(), reinterpret_cast<const render::gl::SRGBA8*>(img.data()));
        texture.texture->image(texture.image->getWidth(), texture.image->getHeight(), texture.image->getData())
            .generateMipmap();

        textures.emplace_back(std::move(texture));
    }
};
} // namespace

TextureAnimator::TextureAnimator(const std::vector<uint16_t>& data,
                                 std::vector<loader::file::TextureTile>& textureTiles,
                                 std::vector<loader::file::DWordTexture>& textures,
                                 bool linear)
{
    int32_t maxSize = 0;
    for(const auto& texture : textures)
        maxSize = std::max(maxSize, texture.image->getWidth());

    Expects(maxSize > 0);

    BOOST_LOG_TRIVIAL(debug) << "Extracting animated texture tiles into " << maxSize << "px texture";
    BOOST_LOG_TRIVIAL(debug) << "  - Collecting animated tile IDs...";

    TextureAtlas atlas{maxSize};
    {
        const uint16_t* ptr = data.data();
        const auto sequenceCount = *ptr++;

        std::set<uint16_t> animatedTiles;
        for(size_t i = 0; i < sequenceCount; ++i)
        {
            Sequence sequence;
            const auto n = *ptr++;
            for(size_t j = 0; j <= n; ++j)
            {
                Expects(ptr <= &data.back());
                const auto tileId = *ptr++;
                if(animatedTiles.emplace(tileId).second)
                {
                    atlas.insert(textureTiles, tileId);
                }
                sequence.tileIds.emplace_back(tileId);
                m_sequenceByTileId.emplace(tileId, m_sequences.size());
            }
            m_sequences.emplace_back(std::move(sequence));
        }
    }

    BOOST_LOG_TRIVIAL(debug) << "  - Laying out texture tiles...";
    atlas.layOutTextures(textures);

    BOOST_LOG_TRIVIAL(debug) << "  - Building texture...";
    atlas.toTexture(textures, textureTiles, linear);
}
}
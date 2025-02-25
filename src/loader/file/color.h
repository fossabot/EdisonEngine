#pragma once

#include "gsl-lite.hpp"
#include "io/sdlreader.h"
#include "render/gl/pixel.h"

namespace loader
{
namespace file
{
/**
* @brief 8-Bit RGBA color.
* @ingroup native
*
* @note The alpha component is set from TR2 on only.
*/
struct ByteColor
{
    uint8_t r = 0, g = 0, b = 0, a = 0;

    static ByteColor readTr1(io::SDLReader& reader)
    {
        return read(reader, false);
    }

    static ByteColor readTr2(io::SDLReader& reader)
    {
        return read(reader, true);
    }

    render::gl::SRGBA8 toTextureColor() const
    {
        return render::gl::SRGBA8{r, g, b, a};
    }

    render::gl::SRGBA8 toTextureColor(uint8_t alphaOverride) const
    {
        return render::gl::SRGBA8{r, g, b, alphaOverride};
    }

    glm::vec4 toGLColor() const
    {
        return glm::vec4{r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    }

    glm::vec3 toGLColor3() const
    {
        return glm::vec3{r / 255.0f, g / 255.0f, b / 255.0f};
    }

private:
    static ByteColor read(io::SDLReader& reader, const bool withAlpha)
    {
        ByteColor color;
        color.r = reader.readU8() << 2u;
        color.g = reader.readU8() << 2u;
        color.b = reader.readU8() << 2u;
        if(withAlpha)
            color.a = reader.readU8() << 2u;
        else
            color.a = 255;
        return color;
    }
};

/**
* @brief 32-Bit float RGBA color.
* @ingroup native
*/
struct FloatColor
{
    float r, g, b, a;

    glm::vec4 toSColor(const float intensity) const
    {
        BOOST_ASSERT(intensity >= 0 && intensity <= 1);
        glm::vec4 col;
        col.x = r * intensity;
        col.y = g * intensity;
        col.z = b * intensity;
        col.w = a * intensity;
        return col;
    }
};

struct Palette
{
    ByteColor colors[256];

    /// \brief reads the 256 color palette values.
    static std::unique_ptr<Palette> readTr1(io::SDLReader& reader)
    {
        std::unique_ptr<Palette> palette{new Palette()};
        for(auto& c : gsl::span<ByteColor>(palette->colors))
            c = ByteColor::readTr1(reader);
        return palette;
    }

    static std::unique_ptr<Palette> readTr2(io::SDLReader& reader)
    {
        std::unique_ptr<Palette> palette{new Palette()};
        for(auto& c : gsl::span<ByteColor>(palette->colors))
            c = ByteColor::readTr2(reader);
        return palette;
    }
};
} // namespace file
} // namespace loader

#pragma once

#include "image.h"
#include "pixel.h"

#include <ft2build.h>
#include FT_CACHE_H

namespace render
{
namespace gl
{
class Font
{
public:
    Font(const Font&) = delete;

    Font(Font&&) noexcept = delete;

    Font& operator=(const Font&) = delete;

    Font& operator=(Font&&) = delete;

    void drawText(const char* text, int x, int y, const SRGBA8& color);

    void drawText(const std::string& text, int x, int y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

    Font(std::string ttf, int size);

    ~Font();

    void setTarget(const std::shared_ptr<Image<SRGBA8>>& img)
    {
        m_targetImage = img;
    }

    const std::shared_ptr<Image<SRGBA8>>& getTarget() const
    {
        return m_targetImage;
    }

private:
    FTC_Manager m_cache = nullptr;

    FTC_CMapCache m_cmapCache = nullptr;

    FTC_SBitCache m_sbitCache = nullptr;

    FTC_ImageTypeRec m_imgType;

    std::shared_ptr<Image<SRGBA8>> m_targetImage = nullptr;

    int m_x0 = 0;

    int m_y0 = 0;

    const std::string m_filename;
};
} // namespace gl
} // namespace render

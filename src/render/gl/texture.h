#pragma once

#include "rendertarget.h"

#include <vector>

namespace render
{
namespace gl
{
class Texture : public RenderTarget
{
protected:
    explicit Texture(::gl::TextureTarget type, const std::string& label = {})
        : RenderTarget{::gl::genTextures,
                       [type](const uint32_t handle) { ::gl::bindTexture(type, handle); },
                       ::gl::deleteTextures,
                       ::gl::ObjectIdentifier::Texture,
                       label}
        , m_type{type}
    {
    }

public:
    Texture& set(const ::gl::TextureMinFilter value)
    {
        bind();
        GL_ASSERT(::gl::texParameter(m_type, ::gl::TextureParameterName::TextureMinFilter, (int32_t)value));
        return *this;
    }

    Texture& set(const ::gl::TextureMagFilter value)
    {
        bind();
        GL_ASSERT(::gl::texParameter(m_type, ::gl::TextureParameterName::TextureMagFilter, (int32_t)value));
        return *this;
    }

    Texture& set(const ::gl::TextureParameterName param, const ::gl::TextureWrapMode value)
    {
        bind();
        GL_ASSERT(::gl::texParameter(m_type, param, (int32_t)value));
        return *this;
    }

    ::gl::TextureTarget getType() const noexcept
    {
        return m_type;
    }

    Texture& generateMipmap()
    {
        bind();
        GL_ASSERT(::gl::generateMipmap(m_type));
        return *this;
    }

private:
    const ::gl::TextureTarget m_type;
};

template<typename PixelT>
class Texture2D : public Texture
{
public:
    explicit Texture2D(const std::string& label = {})
        : Texture{::gl::TextureTarget::Texture2d, label}
    {
    }

    int32_t getWidth() const noexcept override
    {
        return m_width;
    }

    int32_t getHeight() const noexcept override
    {
        return m_height;
    }

    Texture2D<PixelT>& image(int32_t width, int32_t height)
    {
        return image(width, height, std::vector<PixelT>{});
    }

    Texture2D<PixelT>& image(const int32_t width, const int32_t height, const std::vector<PixelT>& data)
    {
        BOOST_ASSERT(width > 0 && height > 0);
        BOOST_ASSERT(data.empty() || static_cast<std::size_t>(width) * static_cast<std::size_t>(height) == data.size());

        bind();

        GL_ASSERT(::gl::texImage2D(getType(),
                                   0,
                                   PixelT::InternalFormat,
                                   width,
                                   height,
                                   0,
                                   PixelT::PixelFormat,
                                   PixelT::PixelType,
                                   data.empty() ? nullptr : data.data()));

        m_width = width;
        m_height = height;

        return *this;
    }

    Texture2D<PixelT>& image(const std::vector<PixelT>& data, uint8_t level = 0)
    {
        Expects(level < 32);
        Expects(m_width > 0 && m_height > 0);
        const int levelDiv = 1 << level;
        Expects(data.empty()
                || static_cast<std::size_t>(m_width / levelDiv) * static_cast<std::size_t>(m_height / levelDiv)
                       == data.size());

        bind();

        GL_ASSERT(::gl::texImage2D(getType(),
                                   level,
                                   PixelT::InternalFormat,
                                   m_width / levelDiv,
                                   m_height / levelDiv,
                                   0,
                                   PixelT::PixelFormat,
                                   PixelT::PixelType,
                                   data.empty() ? nullptr : data.data()));

        return *this;
    }

    Texture2D<PixelT>& image(const PixelT* data, uint8_t level = 0)
    {
        Expects(level < 32);
        Expects(m_width > 0 && m_height > 0);
        const int levelDiv = 1 << level;
        bind();

        GL_ASSERT(::gl::texImage2D(getType(),
                                   level,
                                   PixelT::InternalFormat,
                                   m_width / levelDiv,
                                   m_height / levelDiv,
                                   0,
                                   PixelT::PixelFormat,
                                   PixelT::PixelType,
                                   data));

        return *this;
    }

    Texture2D<PixelT>& subImage(const std::vector<PixelT>& data)
    {
        BOOST_ASSERT(m_width > 0 && m_height > 0);
        BOOST_ASSERT(static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height) == data.size());

        bind();

        GL_ASSERT(::gl::texSubImage2D(
            getType(), 0, 0, 0, m_width, m_height, PixelT::PixelFormat, PixelT::PixelType, data.data()));

        return *this;
    }

    Texture2D<PixelT>& copyImageSubData(const Texture2D& src)
    {
        GL_ASSERT(::gl::copyImageSubData(
            src.getHandle(), src.m_type, 0, 0, 0, 0, getHandle(), m_type, 0, 0, 0, 0, src.m_width, src.m_height, 1));
        m_width = src.m_width;
        m_height = src.m_height;

        return *this;
    }

private:
    int32_t m_width = -1;

    int32_t m_height = -1;
};

class TextureDepth : public Texture
{
public:
    explicit TextureDepth(const std::string& label = {})
        : Texture{::gl::TextureTarget::Texture2d, label}
    {
    }

    int32_t getWidth() const noexcept override
    {
        return m_width;
    }

    int32_t getHeight() const noexcept override
    {
        return m_height;
    }

    TextureDepth& copyImageSubData(const TextureDepth& src)
    {
        GL_ASSERT(::gl::copyImageSubData(src.getHandle(),
                                         src.getType(),
                                         0,
                                         0,
                                         0,
                                         0,
                                         getHandle(),
                                         getType(),
                                         0,
                                         0,
                                         0,
                                         0,
                                         src.m_width,
                                         src.m_height,
                                         1));
        m_width = src.m_width;
        m_height = src.m_height;

        return *this;
    }

    TextureDepth& image(const int32_t width, const int32_t height)
    {
        BOOST_ASSERT(width > 0 && height > 0);

        bind();

        GL_ASSERT(::gl::texImage2D(getType(),
                                   0,
                                   ::gl::InternalFormat::DepthComponent32f,
                                   width,
                                   height,
                                   0,
                                   ::gl::PixelFormat::DepthComponent,
                                   ::gl::PixelType::Float,
                                   nullptr));

        m_width = width;
        m_height = height;

        return *this;
    }

private:
    int32_t m_width = -1;

    int32_t m_height = -1;
};

} // namespace gl
} // namespace render

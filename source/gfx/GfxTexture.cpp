//======================================================================
// gfx/GfxTexture.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxTexture.h"
#include "gfx/GfxRawGL.h"

#include <vector>

namespace gfx
{
    GLuint TextureManager::CreateFromRGBA(int width, int height, const unsigned char* rgba,
                                          bool generateMipmaps)
    {
        if (rgba == nullptr || width <= 0 || height <= 0)
        {
            return 0;
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        if (texture == 0)
        {
            return 0;
        }

        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // Internal format sized: obrigatorio no core profile.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, rgba);

        if (generateMipmaps)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    std::vector<unsigned char> TextureManager::ExpandAlphaToRGBA(int width, int height, const unsigned char* alpha)
    {
        std::vector<unsigned char> rgba;
        if (alpha == nullptr || width <= 0 || height <= 0)
        {
            return rgba;
        }

        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        rgba.assign(pixelCount * 4, 0);
        for (size_t i = 0; i < pixelCount; ++i)
        {
            rgba[i * 4 + 3] = alpha[i];
        }
        return rgba;
    }

    GLuint TextureManager::CreateFromAlpha(int width, int height, const unsigned char* alpha,
                                           bool generateMipmaps)
    {
        if (alpha == nullptr || width <= 0 || height <= 0)
        {
            return 0;
        }

        // Conversao GL_ALPHA -> RGBA mantendo RGB = 0.
        // Uma textura GL_ALPHA devolve (0, 0, 0, cobertura); com RGBA e
        // RGB zerado o resultado no fragment shader e identico.
        const std::vector<unsigned char> rgba = ExpandAlphaToRGBA(width, height, alpha);
        return CreateFromRGBA(width, height, rgba.data(), generateMipmaps);
    }

    void TextureManager::ApplySampling(GLuint texture, GLenum minFilter, GLenum magFilter, GLenum wrapMode)
    {
        if (texture == 0)
        {
            return;
        }

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(minFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(magFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrapMode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrapMode));
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void TextureManager::ApplyAnisotropy(GLuint texture, float maxAnisotropy)
    {
        if (texture == 0 || !GLEW_EXT_texture_filter_anisotropic)
        {
            return;
        }

        GLfloat maxSupported = 1.0f;
        // Funcao CRUA: "glGetFloatv" e redirecionada pelas macros do
        // GfxGLAdapter.h e voltaria para o despachante do modulo gfx.
        gfx::raw::fnGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxSupported);
        if (maxSupported < maxAnisotropy)
        {
            maxAnisotropy = maxSupported;
        }

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void TextureManager::Delete(GLuint texture)
    {
        if (texture != 0)
        {
            glDeleteTextures(1, &texture);
        }
    }
}

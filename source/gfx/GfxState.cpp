//======================================================================
// gfx/GfxState.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxState.h"

namespace gfx
{
    void State::Reset()
    {
        m_TextureEnabled   = true;
        m_DepthTestEnabled = true;
        m_DepthMaskEnabled = true;
        m_CullFaceEnabled  = true;
        m_BlendEnabled     = false;
        m_AlphaTestEnabled = false;
        m_FogEnabled       = false;
        m_ScissorEnabled   = false;
        m_StencilEnabled   = false;
        for (int i = 0; i < 4; ++i)
        {
            m_ColorMask[i] = true;
        }

        m_DepthFunc = GL_LEQUAL;
        m_BlendSrc  = GL_ONE;
        m_BlendDst  = GL_ZERO;
        // ATENCAO: m_AlphaFunc e o enum INTERNO gfx::AlphaFunc (0..7),
        // usado diretamente como uniform uAlphaFunc no shader. Nao e um
        // GLenum - usar GL_GREATER (0x0204) aqui desligava o alpha test.
        m_AlphaFunc = ALPHAFUNC_GREATER;
        m_AlphaRef  = 0.25f;

        m_FogMode    = GL_LINEAR;
        m_FogDensity = 0.0004f;
        m_FogStart   = 2000.0f;
        m_FogEnd     = 2700.0f;
        m_FogColor[0] = 30.0f / 256.0f;
        m_FogColor[1] = 20.0f / 256.0f;
        m_FogColor[2] = 10.0f / 256.0f;
        m_FogColor[3] = 1.0f;

        m_TexEnvMode = TEXENV_MODULATE;

        m_CurColor[0] = m_CurColor[1] = m_CurColor[2] = m_CurColor[3] = 1.0f;
        m_CurNormal[0] = 0.0f; m_CurNormal[1] = 0.0f; m_CurNormal[2] = 1.0f;
        m_CurTexCoord[0] = 0.0f; m_CurTexCoord[1] = 0.0f;

        m_Viewport[0] = m_Viewport[1] = m_Viewport[2] = m_Viewport[3] = 0;
        m_Scissor[0] = m_Scissor[1] = m_Scissor[2] = m_Scissor[3] = 0;

        m_FrontFace = GL_CCW;
        m_PolygonModeFace = GL_FRONT_AND_BACK;
        m_PolygonMode = GL_FILL;
    }

    void State::SetTextureEnabled(bool enabled)      { m_TextureEnabled = enabled; }
    void State::SetDepthTestEnabled(bool enabled)    { m_DepthTestEnabled = enabled; }
    void State::SetDepthMask(bool enabled)           { m_DepthMaskEnabled = enabled; }
    void State::SetDepthFunc(GLenum func)            { m_DepthFunc = func; }
    void State::SetCullFaceEnabled(bool enabled)     { m_CullFaceEnabled = enabled; }
    void State::SetBlendEnabled(bool enabled)        { m_BlendEnabled = enabled; }
    void State::SetAlphaTestEnabled(bool enabled)    { m_AlphaTestEnabled = enabled; }
    void State::SetFogEnabled(bool enabled)          { m_FogEnabled = enabled; }
    void State::SetScissorEnabled(bool enabled)      { m_ScissorEnabled = enabled; }
    void State::SetStencilEnabled(bool enabled)      { m_StencilEnabled = enabled; }

    void State::SetBlendFunc(GLenum src, GLenum dst)
    {
        m_BlendSrc = src;
        m_BlendDst = dst;
    }

    void State::SetColorMask(bool r, bool g, bool b, bool a)
    {
        m_ColorMask[0] = r;
        m_ColorMask[1] = g;
        m_ColorMask[2] = b;
        m_ColorMask[3] = a;
    }

    void State::GetColorMask(bool& r, bool& g, bool& b, bool& a) const
    {
        r = m_ColorMask[0];
        g = m_ColorMask[1];
        b = m_ColorMask[2];
        a = m_ColorMask[3];
    }

    void State::SetAlphaFunc(AlphaFunc func, GLfloat ref)
    {
        m_AlphaFunc = func;
        m_AlphaRef  = ref;
    }

    void State::SetFogColor(const GLfloat color[4])
    {
        m_FogColor[0] = color[0];
        m_FogColor[1] = color[1];
        m_FogColor[2] = color[2];
        m_FogColor[3] = color[3];
    }

    void State::GetFogColor(GLfloat out[4]) const
    {
        out[0] = m_FogColor[0];
        out[1] = m_FogColor[1];
        out[2] = m_FogColor[2];
        out[3] = m_FogColor[3];
    }

    void State::SetFogParams(GLenum mode, GLfloat density, GLfloat start, GLfloat end)
    {
        m_FogMode    = mode;
        m_FogDensity = density;
        m_FogStart   = start;
        m_FogEnd     = end;
    }

    void State::SetTexEnvMode(TexEnvMode mode) { m_TexEnvMode = mode; }

    void State::SetCurrentColor(float r, float g, float b, float a)
    {
        m_CurColor[0] = r;
        m_CurColor[1] = g;
        m_CurColor[2] = b;
        m_CurColor[3] = a;
    }

    void State::SetViewport(int x, int y, int width, int height)
    {
        m_Viewport[0] = x;
        m_Viewport[1] = y;
        m_Viewport[2] = width;
        m_Viewport[3] = height;
    }

    void State::SetScissorBox(int x, int y, int width, int height)
    {
        m_Scissor[0] = x;
        m_Scissor[1] = y;
        m_Scissor[2] = width;
        m_Scissor[3] = height;
    }

    void State::GetScissorBox(int out[4]) const
    {
        out[0] = m_Scissor[0];
        out[1] = m_Scissor[1];
        out[2] = m_Scissor[2];
        out[3] = m_Scissor[3];
    }

    void State::SetFrontFace(GLenum mode) { m_FrontFace = mode; }

    void State::SetPolygonMode(GLenum face, GLenum mode)
    {
        m_PolygonModeFace = face;
        m_PolygonMode = mode;
    }
}

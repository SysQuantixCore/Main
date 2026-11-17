//======================================================================
// gfx/GfxState.h
//
// Cache de estado de render.
//
// Substitui o cache "manual" que existia espalhado por ZzzOpenglUtil.cpp
// (CachTexture, TextureEnable, DepthTestEnable, CullFaceEnable,
// DepthMaskEnable, AlphaTestEnable, AlphaBlendType) por um cache unico e
// completo, que sabe se aplicar em:
//
//   - Backend legado : chamadas GL 1.1 diretas.
//   - Backend moderno: uniforms do shader + chamadas core-profile.
//
// O backend moderno NAO possui:
//   - glEnable(GL_TEXTURE_2D) global  -> vira uniform uUseTexture
//   - glEnable(GL_ALPHA_TEST)         -> vira uniform uUseAlphaTest + discard
//   - glFogf / glFogfv                -> vira uniforms uUseFog/uFog*
//   - glAlphaFunc                     -> vira uniform uAlphaTestRef
//======================================================================
#pragma once

#include "gfx/GfxTypes.h"

namespace gfx
{
    class State
    {
    public:
        void Reset();

        // ---------------- Textura ----------------
        void SetTextureEnabled(bool enabled);
        bool IsTextureEnabled() const { return m_TextureEnabled; }

        // ---------------- Profundidade ----------------
        void SetDepthTestEnabled(bool enabled);
        bool IsDepthTestEnabled() const { return m_DepthTestEnabled; }

        void SetDepthMask(bool enabled);
        bool GetDepthMask() const { return m_DepthMaskEnabled; }

        void SetDepthFunc(GLenum func);
        GLenum GetDepthFunc() const { return m_DepthFunc; }

        // ---------------- Culling ----------------
        void SetCullFaceEnabled(bool enabled);
        bool IsCullFaceEnabled() const { return m_CullFaceEnabled; }

        void SetFrontFace(GLenum mode);
        GLenum GetFrontFace() const { return m_FrontFace; }

        // ---------------- Blend ----------------
        void SetBlendEnabled(bool enabled);
        bool IsBlendEnabled() const { return m_BlendEnabled; }

        void SetBlendFunc(GLenum src, GLenum dst);
        void GetBlendFunc(GLenum& src, GLenum& dst) const { src = m_BlendSrc; dst = m_BlendDst; }

        void SetColorMask(bool r, bool g, bool b, bool a);
        void GetColorMask(bool& r, bool& g, bool& b, bool& a) const;

        // ---------------- Alpha test ----------------
        void SetAlphaTestEnabled(bool enabled);
        bool IsAlphaTestEnabled() const { return m_AlphaTestEnabled; }

        void SetAlphaFunc(AlphaFunc func, GLfloat ref);
        AlphaFunc GetAlphaFunc() const { return m_AlphaFunc; }
        GLfloat GetAlphaRef()  const { return m_AlphaRef; }

        // ---------------- Fog ----------------
        void SetFogEnabled(bool enabled);
        bool IsFogEnabled() const { return m_FogEnabled; }

        void SetFogColor(const GLfloat color[4]);
        void GetFogColor(GLfloat out[4]) const;

        void SetFogParams(GLenum mode, GLfloat density, GLfloat start, GLfloat end);
        GLenum  GetFogMode()    const { return m_FogMode; }
        GLfloat GetFogDensity() const { return m_FogDensity; }
        GLfloat GetFogStart()   const { return m_FogStart; }
        GLfloat GetFogEnd()     const { return m_FogEnd; }

        // ---------------- Environment de textura ----------------
        void SetTexEnvMode(TexEnvMode mode);
        TexEnvMode GetTexEnvMode() const { return m_TexEnvMode; }

        // ---------------- Atributos correntes ----------------
        // Substituem glColor3f/glColor4f/glColor4ub/glNormal3f/glTexCoord2f.
        void SetCurrentColor(float r, float g, float b, float a);
        void GetCurrentColor(float out[4]) const { out[0] = m_CurColor[0]; out[1] = m_CurColor[1]; out[2] = m_CurColor[2]; out[3] = m_CurColor[3]; }

        void SetCurrentNormal(const GLfloat n[3]) { m_CurNormal[0] = n[0]; m_CurNormal[1] = n[1]; m_CurNormal[2] = n[2]; }
        void GetCurrentNormal(GLfloat out[3]) const { out[0] = m_CurNormal[0]; out[1] = m_CurNormal[1]; out[2] = m_CurNormal[2]; }

        void SetCurrentTexCoord(float u, float v) { m_CurTexCoord[0] = u; m_CurTexCoord[1] = v; }
        void GetCurrentTexCoord(float out[2]) const { out[0] = m_CurTexCoord[0]; out[1] = m_CurTexCoord[1]; }

        // ---------------- Viewport / scissor ----------------
        void SetViewport(int x, int y, int width, int height);
        void GetViewport(int out[4]) const { out[0] = m_Viewport[0]; out[1] = m_Viewport[1]; out[2] = m_Viewport[2]; out[3] = m_Viewport[3]; }

        void SetScissorEnabled(bool enabled);
        bool IsScissorEnabled() const { return m_ScissorEnabled; }
        void SetScissorBox(int x, int y, int width, int height);
        void GetScissorBox(int out[4]) const;

        // ---------------- Stencil ----------------
        void SetStencilEnabled(bool enabled);
        bool IsStencilEnabled() const { return m_StencilEnabled; }

        // ---------------- Polygon mode ----------------
        void SetPolygonMode(GLenum face, GLenum mode);

    private:
        bool  m_TextureEnabled    = true;
        bool  m_DepthTestEnabled  = true;
        bool  m_DepthMaskEnabled  = true;
        bool  m_CullFaceEnabled   = true;
        bool  m_BlendEnabled      = false;
        bool  m_AlphaTestEnabled  = false;
        bool  m_FogEnabled        = false;
        bool  m_ScissorEnabled    = false;
        bool  m_StencilEnabled    = false;
        bool  m_ColorMask[4]      = { true, true, true, true };

        GLenum m_DepthFunc  = GL_LEQUAL;
        GLenum m_BlendSrc   = GL_ONE;
        GLenum m_BlendDst   = GL_ZERO;
        AlphaFunc m_AlphaFunc = ALPHAFUNC_GREATER;
        GLfloat m_AlphaRef  = 0.25f;

        GLenum  m_FogMode    = GL_LINEAR;
        GLfloat m_FogDensity = 0.0004f;
        GLfloat m_FogStart   = 2000.0f;
        GLfloat m_FogEnd     = 2700.0f;
        GLfloat m_FogColor[4] = { 30.0f / 256.0f, 20.0f / 256.0f, 10.0f / 256.0f, 1.0f };

        TexEnvMode m_TexEnvMode = TEXENV_MODULATE;

        GLfloat m_CurColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat m_CurNormal[3] = { 0.0f, 0.0f, 1.0f };
        float   m_CurTexCoord[2] = { 0.0f, 0.0f };

        int m_Viewport[4] = { 0, 0, 0, 0 };
        int m_Scissor[4]  = { 0, 0, 0, 0 };

        GLenum m_FrontFace = GL_CCW;
        GLenum m_PolygonModeFace = GL_FRONT_AND_BACK;
        GLenum m_PolygonMode = GL_FILL;
    };
}

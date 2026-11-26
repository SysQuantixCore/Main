//======================================================================
// gfx/GfxGLAdapter.cpp
//
// Implementacao dos despachantes de compatibilidade.
//
// Regras:
//   - Backend legado  -> chama a OpenGL original (gfx::raw::*).
//   - Backend moderno -> encaminha para gfx::Renderer.
//======================================================================
#include "stdafx.h"

// As macros de redirecionamento serao desativadas neste arquivo para que
// seja possivel capturar os enderecos reais das funcoes OpenGL 1.1.
#undef glBegin
#undef glEnd
#undef glVertex2f
#undef glVertex3f
#undef glVertex3fv
#undef glTexCoord2f
#undef glTexCoord2fv
#undef glColor3f
#undef glColor4f
#undef glColor3fv
#undef glColor4fv
#undef glColor3ub
#undef glColor4ub
#undef glNormal3f
#undef glNormal3fv
#undef glEnableClientState
#undef glDisableClientState
#undef glVertexPointer
#undef glTexCoordPointer
#undef glColorPointer
#undef glDrawArrays
#undef glMatrixMode
#undef glPushMatrix
#undef glPopMatrix
#undef glLoadIdentity
#undef glLoadMatrixf
#undef glMultMatrixf
#undef glRotatef
#undef glTranslatef
#undef glScalef
#undef gluPerspective
#undef gluOrtho2D
#undef glEnable
#undef glDisable
#undef glAlphaFunc
#undef glFogf
#undef glFogfv
#undef glTexEnvi
#undef glTexEnvf
#undef glGetFloatv
#undef glReadPixels
#undef glBindTexture
#undef glBlendFunc
#undef glDepthFunc
#undef glDepthMask
#undef glViewport
#undef glScissor
#undef glColorMask
#undef glFrontFace
#undef glClear

#include "gfx/GfxGLAdapter.h"
#include "gfx/GfxRenderer.h"
#include "gfx/GfxConfig.h"
#include "gfx/GfxTrace.h"

// ---------------------------------------------------------------------
// Ponteiros para as funcoes OpenGL 1.1 originais
// ---------------------------------------------------------------------
namespace gfx
{
    namespace raw
    {
        void (APIENTRY *fnBegin)(GLenum mode)                                = &glBegin;
        void (APIENTRY *fnEnd)(void)                                        = &glEnd;
        void (APIENTRY *fnVertex2f)(GLfloat x, GLfloat y)                   = &glVertex2f;
        void (APIENTRY *fnVertex3f)(GLfloat x, GLfloat y, GLfloat z)        = &glVertex3f;
        void (APIENTRY *fnVertex3fv)(const GLfloat* v)                      = &glVertex3fv;
        void (APIENTRY *fnTexCoord2f)(GLfloat s, GLfloat t)                 = &glTexCoord2f;
        void (APIENTRY *fnTexCoord2fv)(const GLfloat* v)                    = &glTexCoord2fv;
        void (APIENTRY *fnColor3f)(GLfloat r, GLfloat g, GLfloat b)         = &glColor3f;
        void (APIENTRY *fnColor4f)(GLfloat r, GLfloat g, GLfloat b, GLfloat a) = &glColor4f;
        void (APIENTRY *fnColor3fv)(const GLfloat* v)                       = &glColor3fv;
        void (APIENTRY *fnColor4fv)(const GLfloat* v)                       = &glColor4fv;
        void (APIENTRY *fnColor3ub)(GLubyte r, GLubyte g, GLubyte b)        = &glColor3ub;
        void (APIENTRY *fnColor4ub)(GLubyte r, GLubyte g, GLubyte b, GLubyte a) = &glColor4ub;
        void (APIENTRY *fnNormal3f)(GLfloat x, GLfloat y, GLfloat z)        = &glNormal3f;
        void (APIENTRY *fnNormal3fv)(const GLfloat* v)                      = &glNormal3fv;

        void (APIENTRY *fnEnableClientState)(GLenum array)                  = &glEnableClientState;
        void (APIENTRY *fnDisableClientState)(GLenum array)                 = &glDisableClientState;
        void (APIENTRY *fnVertexPointer)(GLint size, GLenum type, GLsizei stride, const void* p) = &glVertexPointer;
        void (APIENTRY *fnTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const void* p) = &glTexCoordPointer;
        void (APIENTRY *fnColorPointer)(GLint size, GLenum type, GLsizei stride, const void* p) = &glColorPointer;
        void (APIENTRY *fnDrawArrays)(GLenum mode, GLint first, GLsizei count) = &glDrawArrays;

        void (APIENTRY *fnMatrixMode)(GLenum mode)                          = &glMatrixMode;
        void (APIENTRY *fnPushMatrix)(void)                                 = &glPushMatrix;
        void (APIENTRY *fnPopMatrix)(void)                                  = &glPopMatrix;
        void (APIENTRY *fnLoadIdentity)(void)                               = &glLoadIdentity;
        void (APIENTRY *fnLoadMatrixf)(const GLfloat* m)                    = &glLoadMatrixf;
        void (APIENTRY *fnMultMatrixf)(const GLfloat* m)                    = &glMultMatrixf;
        void (APIENTRY *fnRotatef)(GLfloat a, GLfloat x, GLfloat y, GLfloat z) = &glRotatef;
        void (APIENTRY *fnTranslatef)(GLfloat x, GLfloat y, GLfloat z)     = &glTranslatef;
        void (APIENTRY *fnScalef)(GLfloat x, GLfloat y, GLfloat z)          = &glScalef;

        void (APIENTRY *fnPerspective)(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar) = &gluPerspective;
        void (APIENTRY *fnOrtho2D)(GLdouble l, GLdouble r, GLdouble b, GLdouble t) = &gluOrtho2D;

        void (APIENTRY *fnEnable)(GLenum cap)                               = &glEnable;
        void (APIENTRY *fnDisable)(GLenum cap)                              = &glDisable;
        void (APIENTRY *fnAlphaFunc)(GLenum func, GLclampf ref)             = &glAlphaFunc;
        void (APIENTRY *fnFogf)(GLenum pname, GLfloat param)                = &glFogf;
        void (APIENTRY *fnFogfv)(GLenum pname, const GLfloat* params)       = &glFogfv;
        void (APIENTRY *fnTexEnvi)(GLenum target, GLenum pname, GLint param) = &glTexEnvi;
        void (APIENTRY *fnTexEnvf)(GLenum target, GLenum pname, GLfloat param) = &glTexEnvf;
        void (APIENTRY *fnGetFloatv)(GLenum pname, GLfloat* params)         = &glGetFloatv;
        void (APIENTRY *fnGetIntegerv)(GLenum pname, GLint* params)       = &glGetIntegerv;
        GLboolean (APIENTRY *fnIsEnabled)(GLenum cap)                  = &glIsEnabled;
        void (APIENTRY *fnReadPixels)(GLint x, GLint y, GLsizei w, GLsizei h,
                                    GLenum format, GLenum type, void* pixels) = &glReadPixels;

        // Estado cru - interceptado apenas para invalidar o lote de batching.
        void (APIENTRY *fnBindTexture)(GLenum t, GLuint tex)             = &glBindTexture;
        void (APIENTRY *fnBlendFunc)(GLenum s, GLenum d)                 = &glBlendFunc;
        void (APIENTRY *fnDepthFunc)(GLenum f)                           = &glDepthFunc;
        void (APIENTRY *fnDepthMask)(GLboolean f)                        = &glDepthMask;
        void (APIENTRY *fnViewport)(GLint x, GLint y, GLsizei w, GLsizei h) = &glViewport;
        void (APIENTRY *fnScissor)(GLint x, GLint y, GLsizei w, GLsizei h) = &glScissor;
        void (APIENTRY *fnColorMask)(GLboolean r, GLboolean g, GLboolean b, GLboolean a) = &glColorMask;
        void (APIENTRY *fnFrontFace)(GLenum m)                           = &glFrontFace;
        void (APIENTRY *fnClear)(GLbitfield m)                           = &glClear;
    }
}

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------
namespace
{
    inline bool gfxModern()
    {
        return gfx::Renderer::Instance().IsModern();
    }

    inline gfx::State& gfxState()
    {
        return gfx::Renderer::Instance().GetState();
    }
}

// ---------------------------------------------------------------------
// Modo imediato
// ---------------------------------------------------------------------
void gfxGLBegin(GLenum mode)
{
    if (gfxModern()) { gfx::Renderer::Instance().Begin(mode); return; }
    gfx::raw::fnBegin(mode);
}

void gfxGLEnd()
{
    if (gfxModern()) { gfx::Renderer::Instance().End(); return; }
    gfx::raw::fnEnd();
}

void gfxGLVertex2f(GLfloat x, GLfloat y)
{
    if (gfxModern()) { gfx::Renderer::Instance().Vertex2f(x, y); return; }
    gfx::raw::fnVertex2f(x, y);
}

void gfxGLVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    if (gfxModern()) { gfx::Renderer::Instance().Vertex3f(x, y, z); return; }
    gfx::raw::fnVertex3f(x, y, z);
}

void gfxGLVertex3fv(const GLfloat* v)
{
    if (gfxModern()) { gfx::Renderer::Instance().Vertex3fv(v); return; }
    gfx::raw::fnVertex3fv(v);
}

void gfxGLTexCoord2f(GLfloat s, GLfloat t)
{
    if (gfxModern()) { gfx::Renderer::Instance().TexCoord2f(s, t); return; }
    gfx::raw::fnTexCoord2f(s, t);
}

void gfxGLTexCoord2fv(const GLfloat* v)
{
    if (gfxModern()) { gfx::Renderer::Instance().TexCoord2fv(v); return; }
    gfx::raw::fnTexCoord2fv(v);
}

void gfxGLColor3f(GLfloat r, GLfloat g, GLfloat b)
{
    if (gfxModern()) { gfx::Renderer::Instance().SetColor3f(r, g, b); return; }
    gfx::raw::fnColor3f(r, g, b);
}

void gfxGLColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    if (gfxModern()) { gfx::Renderer::Instance().SetColor4f(r, g, b, a); return; }
    gfx::raw::fnColor4f(r, g, b, a);
}

void gfxGLColor3fv(const GLfloat* v)
{
    if (gfxModern()) { gfx::Renderer::Instance().SetColor3fv(v); return; }
    gfx::raw::fnColor3fv(v);
}

void gfxGLColor4fv(const GLfloat* v)
{
    if (gfxModern()) { gfx::Renderer::Instance().SetColor4fv(v); return; }
    gfx::raw::fnColor4fv(v);
}

void gfxGLColor3ub(GLubyte r, GLubyte g, GLubyte b)
{
    if (gfxModern()) { gfx::Renderer::Instance().SetColor3ub(r, g, b); return; }
    gfx::raw::fnColor3ub(r, g, b);
}

void gfxGLColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    if (gfxModern()) { gfx::Renderer::Instance().SetColor4ub(r, g, b, a); return; }
    gfx::raw::fnColor4ub(r, g, b, a);
}

void gfxGLNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
    if (gfxModern()) { gfx::Renderer::Instance().Normal3f(x, y, z); return; }
    gfx::raw::fnNormal3f(x, y, z);
}

void gfxGLNormal3fv(const GLfloat* v)
{
    if (gfxModern()) { gfx::Renderer::Instance().Normal3fv(v); return; }
    gfx::raw::fnNormal3fv(v);
}

// ---------------------------------------------------------------------
// Client-state arrays
// ---------------------------------------------------------------------
void gfxGLEnableClientState(GLenum array)
{
    if (gfxModern())
    {
        gfx::Renderer::Instance().EnableClientState(array);
        return;
    }
    gfx::raw::fnEnableClientState(array);
}

void gfxGLDisableClientState(GLenum array)
{
    if (gfxModern()) { gfx::Renderer::Instance().DisableClientState(array); return; }
    gfx::raw::fnDisableClientState(array);
}

void gfxGLVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    if (gfxModern()) { gfx::Renderer::Instance().VertexPointer(size, type, stride, pointer); return; }
    gfx::raw::fnVertexPointer(size, type, stride, pointer);
}

void gfxGLTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    if (gfxModern()) { gfx::Renderer::Instance().TexCoordPointer(size, type, stride, pointer); return; }
    gfx::raw::fnTexCoordPointer(size, type, stride, pointer);
}

void gfxGLColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    if (gfxModern()) { gfx::Renderer::Instance().ColorPointer(size, type, stride, pointer); return; }
    gfx::raw::fnColorPointer(size, type, stride, pointer);
}

void gfxGLDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    if (gfxModern()) { gfx::Renderer::Instance().DrawArrays(mode, first, count); return; }

    gfx::raw::fnDrawArrays(mode, first, count);
}

// ---------------------------------------------------------------------
// Pilha de matrizes
// ---------------------------------------------------------------------
void gfxGLMatrixMode(GLenum mode)
{
    if (gfxModern())
    {
        switch (mode)
        {
        case GL_PROJECTION: gfx::Renderer::Instance().SetMatrixMode(gfx::MatrixMode::Projection); break;
        case GL_TEXTURE:    gfx::Renderer::Instance().SetMatrixMode(gfx::MatrixMode::Texture);    break;
        case GL_MODELVIEW:
        default:             gfx::Renderer::Instance().SetMatrixMode(gfx::MatrixMode::ModelView);  break;
        }
        return;
    }
    gfx::raw::fnMatrixMode(mode);
}

void gfxGLPushMatrix()
{
    if (gfxModern()) { gfx::Renderer::Instance().PushMatrix(); return; }
    gfx::raw::fnPushMatrix();
}

void gfxGLPopMatrix()
{
    if (gfxModern()) { gfx::Renderer::Instance().PopMatrix(); return; }
    gfx::raw::fnPopMatrix();
}

void gfxGLLoadIdentity()
{
    if (gfxModern()) { gfx::Renderer::Instance().LoadIdentity(); return; }
    gfx::raw::fnLoadIdentity();
}

void gfxGLLoadMatrixf(const GLfloat* m)
{
    if (gfxModern()) { gfx::Renderer::Instance().LoadMatrix(m); return; }
    gfx::raw::fnLoadMatrixf(m);
}

void gfxGLMultMatrixf(const GLfloat* m)
{
    if (gfxModern()) { gfx::Renderer::Instance().MultMatrix(m); return; }
    gfx::raw::fnMultMatrixf(m);
}

void gfxGLRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    if (gfxModern()) { gfx::Renderer::Instance().Rotatef(angle, x, y, z); return; }
    gfx::raw::fnRotatef(angle, x, y, z);
}

void gfxGLTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    if (gfxModern()) { gfx::Renderer::Instance().Translatef(x, y, z); return; }
    gfx::raw::fnTranslatef(x, y, z);
}

void gfxGLScalef(GLfloat x, GLfloat y, GLfloat z)
{
    if (gfxModern()) { gfx::Renderer::Instance().Scalef(x, y, z); return; }
    gfx::raw::fnScalef(x, y, z);
}

// ---------------------------------------------------------------------
// Projecao
// ---------------------------------------------------------------------
void gfxGLPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
{
    if (gfxModern())
    {
        gfx::Renderer::Instance().SetProjectionPerspective(
            static_cast<float>(fovy), static_cast<float>(aspect),
            static_cast<float>(zNear), static_cast<float>(zFar));
        return;
    }
    gfx::raw::fnPerspective(fovy, aspect, zNear, zFar);

}

void gfxGLOrtho2D(GLdouble l, GLdouble r, GLdouble b, GLdouble t)
{
    if (gfxModern())
    {
        gfx::Renderer::Instance().SetProjectionOrtho2D(
            static_cast<float>(l), static_cast<float>(r),
            static_cast<float>(b), static_cast<float>(t));
        return;
    }
    gfx::raw::fnOrtho2D(l, r, b, t);

}

// ---------------------------------------------------------------------
// Estado
// ---------------------------------------------------------------------
namespace { void gfxFlushBatchForState(); }
void gfxGLEnable(GLenum cap)
{
    gfxFlushBatchForState();
    if (gfxModern())
    {
        switch (cap)
        {
        case GL_TEXTURE_2D:
            gfxState().SetTextureEnabled(true);
            return;
        case GL_ALPHA_TEST:
            gfxState().SetAlphaTestEnabled(true);
            return;
        case GL_FOG:
            gfxState().SetFogEnabled(true);
            return;
        default:
            break;
        }
    }
    gfx::raw::fnEnable(cap);
}

void gfxGLDisable(GLenum cap)
{
    gfxFlushBatchForState();
    if (gfxModern())
    {
        switch (cap)
        {
        case GL_TEXTURE_2D:
            gfxState().SetTextureEnabled(false);
            return;
        case GL_ALPHA_TEST:
            gfxState().SetAlphaTestEnabled(false);
            return;
        case GL_FOG:
            gfxState().SetFogEnabled(false);
            return;
        default:
            break;
        }
    }
    gfx::raw::fnDisable(cap);
}

void gfxGLAlphaFunc(GLenum func, GLclampf ref)
{
    gfxFlushBatchForState();
    if (gfxModern())
    {
        // Converte o GLenum da OpenGL para o enum interno gfx::AlphaFunc
        // (0..7) que o shader usa diretamente como uAlphaFunc.
        gfx::AlphaFunc af = gfx::ALPHAFUNC_GREATER;
        switch (func)
        {
        case GL_NEVER:    af = gfx::ALPHAFUNC_NEVER;    break;
        case GL_LESS:     af = gfx::ALPHAFUNC_LESS;     break;
        case GL_EQUAL:    af = gfx::ALPHAFUNC_EQUAL;    break;
        case GL_LEQUAL:   af = gfx::ALPHAFUNC_LEQUAL;   break;
        case GL_GREATER:  af = gfx::ALPHAFUNC_GREATER;  break;
        case GL_NOTEQUAL: af = gfx::ALPHAFUNC_NOTEQUAL; break;
        case GL_GEQUAL:   af = gfx::ALPHAFUNC_GEQUAL;   break;
        case GL_ALWAYS:   af = gfx::ALPHAFUNC_ALWAYS;   break;
        default: break;
        }
        gfxState().SetAlphaFunc(af, static_cast<GLfloat>(ref));
        return;
    }
    gfx::raw::fnAlphaFunc(func, ref);
}

void gfxGLFogf(GLenum pname, GLfloat param)
{
    gfxFlushBatchForState();
    if (gfxModern())
    {
        gfx::State& state = gfxState();
        switch (pname)
        {
        case GL_FOG_MODE:    state.SetFogParams(static_cast<GLenum>(param), state.GetFogDensity(), state.GetFogStart(), state.GetFogEnd()); break;
        case GL_FOG_DENSITY: state.SetFogParams(state.GetFogMode(), param, state.GetFogStart(), state.GetFogEnd()); break;
        case GL_FOG_START:   state.SetFogParams(state.GetFogMode(), state.GetFogDensity(), param, state.GetFogEnd()); break;
        case GL_FOG_END:     state.SetFogParams(state.GetFogMode(), state.GetFogDensity(), state.GetFogStart(), param); break;
        default: break;
        }
        return;
    }
    gfx::raw::fnFogf(pname, param);
}

void gfxGLFogfv(GLenum pname, const GLfloat* params)
{
    gfxFlushBatchForState();
    if (gfxModern())
    {
        if (pname == GL_FOG_COLOR && params != nullptr)
        {
            gfxState().SetFogColor(params);
            return;
        }
        // Os demais parametros de fog sao escalares e tratados em Fogf().
        return;
    }
    gfx::raw::fnFogfv(pname, params);
}

void gfxGLTexEnvi(GLenum target, GLenum pname, GLint param)
{
    gfxFlushBatchForState();
    if (gfxModern())
    {
        if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
        {
            switch (param)
            {
            case GL_MODULATE: gfxState().SetTexEnvMode(gfx::TEXENV_MODULATE); break;
            case GL_ADD:      gfxState().SetTexEnvMode(gfx::TEXENV_ADD);      break;
            case GL_REPLACE:  gfxState().SetTexEnvMode(gfx::TEXENV_REPLACE);  break;
            default: break;
            }
            return;
        }
    }
    gfx::raw::fnTexEnvi(target, pname, param);
}

void gfxGLTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    gfxFlushBatchForState();
    if (gfxModern())
    {
        if (target == GL_TEXTURE_ENV && pname == GL_TEXTURE_ENV_MODE)
        {
            gfxGLTexEnvi(target, pname, static_cast<GLint>(param));
            return;
        }
    }
    gfx::raw::fnTexEnvf(target, pname, param);
}

void gfxGLGetFloatv(GLenum pname, GLfloat* params)
{
    if (gfxModern() && params != nullptr)
    {
        if (pname == GL_CURRENT_COLOR)
        {
            float color[4];
            gfx::Renderer::Instance().GetCurrentColor(color);
            params[0] = color[0];
            params[1] = color[1];
            params[2] = color[2];
            params[3] = color[3];
            return;
        }
        if (pname == GL_MODELVIEW_MATRIX)
        {
            const float* data = glm::value_ptr(gfx::Renderer::Instance().GetModelViewMatrix());
            for (int i = 0; i < 16; ++i)
            {
                params[i] = data[i];
            }
            return;
        }
    }
    gfx::raw::fnGetFloatv(pname, params);
}

void gfxGLReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                     GLenum format, GLenum type, void* pixels)
{
    if (gfxModern())
    {
        // Em core profile nao e possivel ler o depth buffer do framebuffer
        // padrao com glReadPixels. O renderer moderno mantem uma copia do
        // depth em uma textura (GfxRenderer::ReadDepth).
        //
        // Em perfil de COMPATIBILIDADE (o padrao do modo moderno) a leitura
        // direta e valida e muito mais barata: o espelho exigia um blit +
        // glGetTexImage (sincronizacao GPU->CPU) por chamada, e o motor
        // consulta o depth uma vez por frame para a visibilidade do sol.
        if (format == GL_DEPTH_COMPONENT && type == GL_FLOAT &&
            gfx::Config::Instance().GetCoreProfile())
        {
            gfx::Renderer::Instance().ReadDepth(x, y, width, height, static_cast<GLfloat*>(pixels));
            return;
        }
        if (format == GL_RGB)
        {
            // O core profile exige GL_RGBA para glReadPixels.
            // Converte para RGB depois da leitura.
            std::vector<GLubyte> rgba(static_cast<size_t>(width) * height * 4, 0);
            gfx::raw::fnReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
            GLubyte* dst = static_cast<GLubyte*>(pixels);
            const size_t count = static_cast<size_t>(width) * height;
            for (size_t i = 0; i < count; ++i)
            {
                dst[i * 3 + 0] = rgba[i * 4 + 0];
                dst[i * 3 + 1] = rgba[i * 4 + 1];
                dst[i * 3 + 2] = rgba[i * 4 + 2];
            }
            return;
        }
    }
    gfx::raw::fnReadPixels(x, y, width, height, format, type, pixels);
}

// ---------------------------------------------------------------------
// Estado cru: fecha o lote de batching antes de aplicar a mudanca.
// ---------------------------------------------------------------------
namespace
{
    inline void gfxFlushBatchForState()
    {
        // So ha lote para fechar se o batching estiver LIGADO. Sem essa
        // verificacao, cada BindTexture/BlendFunc/Enable (centenas por frame)
        // pagava o custo de InvalidateBatch() mesmo com Batch=0.
        if (gfxModern() && gfx::Renderer::Instance().IsBatching())
        {
            gfx::Renderer::Instance().InvalidateBatch();
        }
    }
}

void gfxGLBindTexture(GLenum target, GLuint texture)
{
    gfxFlushBatchForState();
    if (gfxModern() && gfx::Renderer::Instance().IsBindless() &&
        target == GL_TEXTURE_2D)
    {
        // Bindless: registra o nome da textura no renderer. O glBindTexture
        // real continua sendo chamado abaixo para manter o binding do driver
        // sincronizado (o jogo tem cache CachTexture e as vezes nao chama
        // glBindTexture quando a textura muda logicamente).
        gfx::Renderer::Instance().SetTexture(texture);
    }
    gfx::raw::fnBindTexture(target, texture);
}

void gfxGLBlendFunc(GLenum sfactor, GLenum dfactor)
{
    gfxFlushBatchForState();
    gfx::raw::fnBlendFunc(sfactor, dfactor);
}

void gfxGLDepthFunc(GLenum func)
{
    gfxFlushBatchForState();
    gfx::raw::fnDepthFunc(func);
}

void gfxGLDepthMask(GLboolean flag)
{
    gfxFlushBatchForState();
    gfx::raw::fnDepthMask(flag);
}

void gfxGLViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    gfxFlushBatchForState();
    gfx::raw::fnViewport(x, y, width, height);
}

void gfxGLScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    gfxFlushBatchForState();
    gfx::raw::fnScissor(x, y, width, height);
}

void gfxGLColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    gfxFlushBatchForState();
    gfx::raw::fnColorMask(r, g, b, a);
}

void gfxGLFrontFace(GLenum mode)
{
    gfxFlushBatchForState();
    gfx::raw::fnFrontFace(mode);
}

void gfxGLClear(GLbitfield mask)
{
    gfxFlushBatchForState();
    if (gfxModern())
    {
        // Inicio de frame: reinicia o cursor do buffer mapeado. So faz sentido
        // se o clear incluir cor (o clear de profundidade no meio da cena nao
        // deve resetar o cursor).
        if (mask & GL_COLOR_BUFFER_BIT)
        {
            gfx::Renderer::Instance().GetBuffer().BeginFrame();
        }
    }
    gfx::raw::fnClear(mask);
}

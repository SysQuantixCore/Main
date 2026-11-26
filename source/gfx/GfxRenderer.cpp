//======================================================================
// gfx/GfxRenderer.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxRenderer.h"
#include "gfx/GfxShaders.h"
#include "gfx/GfxTrace.h"
#include "gfx/GfxConfig.h"

// Cache legado do motor (ZzzOpenglUtil.h). Usado apenas no trace de
// diagnostico para comparar com o estado do modulo gfx.
extern bool TextureEnable;
extern bool AlphaTestEnable;

namespace gfx
{
    namespace
    {
        // Converte o modo de primitiva legado para o equivalente suportado
        // pelo core profile. Retorna 0 se o modo nao for suportado.
        GLenum ToCorePrimitive(GLenum mode)
        {
            switch (mode)
            {
            case GL_TRIANGLES: return GL_TRIANGLES;
            case GL_LINES:     return GL_LINES;
            case GL_LINE_STRIP:return GL_LINE_STRIP;
            case GL_LINE_LOOP:  return GL_LINE_LOOP;
            case GL_POINTS:     return GL_POINTS;
            default:            return 0;
            }
        }

        // GL_QUADS e GL_TRIANGLE_FAN nao existem no core profile.
        bool NeedsTriangleExpansion(GLenum mode)
        {
            return mode == GL_QUADS || mode == GL_TRIANGLE_FAN || mode == GL_POLYGON;
        }
    }

    Renderer& Renderer::Instance()
    {
        static Renderer instance;
        return instance;
    }

    bool Renderer::InitializeModern(std::string& errorLog)
    {
        Shutdown();

        // Fase 2: usa bindless se o driver suportar e a config permitir.
        // Tenta compilar o shader bindless; se falhar, cai para o classico.
        m_UseBindless = Config::Instance().GetBindless() && (GLEW_ARB_bindless_texture != 0);
        const char* fragment = m_UseBindless ? kFragmentShaderBindless : kFragmentShaderSource;
        std::string bindlessError;
        if (m_UseBindless && !m_Shader.Compile(kVertexShaderSource, fragment, bindlessError))
        {
            // Shader bindless nao compilou -> fallback classico (sem perder a mensagem original).
            m_UseBindless = false;
            fragment = kFragmentShaderSource;
        }
        if (!m_Shader.Compile(kVertexShaderSource, fragment, errorLog))
        {
            return false;
        }
        m_TextureHandles.clear();
        m_CurrentTextureHandle = 0;

        // O sampler classico le da unidade 0 (uma unica vez). No caminho
        // bindless o uniform e uTextureHandle, definido por draw.
        m_Shader.Use();
        if (!m_UseBindless)
        {
            m_Shader.SetInt("uTexture", 0);
        }

        m_Buffer.SetMapped(Config::Instance().GetMapped());
        // 4 regioes de 32768 vertices cada (131072 total): cada regiao comporta
        // um frame inteiro (~6000 vertices expandidos) com folga.
        if (!m_Buffer.Create(131072))
        {
            errorLog = "Falha ao criar VAO/VBO do renderer moderno.\r\n";
            m_Shader.Release();
            return false;
        }

        m_State.Reset();
        m_ModelView = MatrixStack();
        m_Projection = MatrixStack();
        m_Texture = MatrixStack();
        m_CurrentMode = MatrixMode::ModelView;

        m_Staging.clear();
        m_Staging.reserve(4096 * VERTEX_FLOATS);
        m_Batch.clear();
        m_Batch.reserve(8192 * VERTEX_FLOATS);
        m_BatchEnabled = Config::Instance().GetBatch();
        m_ClientArray = Config::Instance().GetClientArray();
        m_Buffer.SetClientArray(m_ClientArray);
        m_StateGen = 0;
        m_BatchGen = 0;

        m_UniformsDirty = true;
        m_Modern = true;
        return true;
    }

    void Renderer::Shutdown()
    {
        if (m_DepthFbo != 0)
        {
            glDeleteFramebuffers(1, &m_DepthFbo);
            m_DepthFbo = 0;
        }
        if (m_DepthTexture != 0)
        {
            glDeleteTextures(1, &m_DepthTexture);
            m_DepthTexture = 0;
        }
        m_DepthTextureFormat = 0;

        m_Buffer.Release();
        m_Shader.Release();
        m_Staging.clear();
        m_Modern = false;
    }

    // -----------------------------------------------------------------
    // Leitura do depth buffer
    // -----------------------------------------------------------------

    bool Renderer::EnsureDepthMirror()
    {
        if (m_DepthFbo != 0 && m_DepthTexture != 0)
        {
            return true;
        }

        // A textura precisa ter o mesmo formato de profundidade do
        // framebuffer padrao para que o blit seja valido.
        GLint depthBits = 24;
        glGetIntegerv(GL_DEPTH_BITS, &depthBits);

        GLenum format = GL_DEPTH_COMPONENT24;
        if (depthBits <= 16)
        {
            format = GL_DEPTH_COMPONENT16;
        }
        else if (depthBits >= 32)
        {
            format = GL_DEPTH_COMPONENT32F;
        }

        glGenTextures(1, &m_DepthTexture);
        if (m_DepthTexture == 0)
        {
            return false;
        }

        glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &m_DepthFbo);
        if (m_DepthFbo == 0)
        {
            glDeleteTextures(1, &m_DepthTexture);
            m_DepthTexture = 0;
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_DepthFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);
        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            glDeleteFramebuffers(1, &m_DepthFbo);
            m_DepthFbo = 0;
            glDeleteTextures(1, &m_DepthTexture);
            m_DepthTexture = 0;
            return false;
        }

        m_DepthTextureFormat = format;
        return true;
    }

    bool Renderer::ReadDepth(GLint x, GLint y, GLsizei width, GLsizei height, GLfloat* out)
    {
        if (out == nullptr || width <= 0 || height <= 0)
        {
            return false;
        }

        if (!EnsureDepthMirror())
        {
            return false;
        }

        // 1) Copia a regiao do depth buffer do framebuffer padrao para a
        //    textura 1x1 do espelho.
        GLint previousRead = 0, previousDraw = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousRead);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDraw);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_DepthFbo);
        glBlitFramebuffer(x, y, x + width, y + height,
                          0, 0, width, height,
                          GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        const GLenum error = glGetError();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousRead));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDraw));

        if (error != GL_NO_ERROR)
        {
            if (!m_DepthMirrorWarned)
            {
                m_DepthMirrorWarned = true;
                OutputDebugStringA("[gfx] Aviso: leitura do depth buffer indisponivel neste driver.\r\n");
            }
            return false;
        }

        // 2) Le a textura de profundidade (formato valido em core profile).
        std::vector<GLfloat> buffer(static_cast<size_t>(width) * height, 1.0f);

        glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, buffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        if (glGetError() != GL_NO_ERROR)
        {
            if (!m_DepthMirrorWarned)
            {
                m_DepthMirrorWarned = true;
                OutputDebugStringA("[gfx] Aviso: glGetTexImage do depth buffer falhou.\r\n");
            }
            return false;
        }

        for (size_t i = 0; i < buffer.size(); ++i)
        {
            out[i] = buffer[i];
        }
        return true;
    }

    GLuint64 Renderer::SetTexture(GLuint name)
    {
        // Guarda o nome; o handle e criado lazy no primeiro desenho, quando
        // a textura ja estiver completa (glGetTextureHandleARB retorna 0 para
        // texturas incompletas).
        m_CurrentBoundName = name;
        return 0;
    }

    GLint Renderer::currentProgram()
    {
        GLint program = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        return program;
    }


    // -----------------------------------------------------------------
    // Pilhas de matrizes
    // -----------------------------------------------------------------

    void Renderer::SetMatrixMode(MatrixMode mode)
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        m_CurrentMode = mode;
    }

    void Renderer::PushMatrix()
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        switch (m_CurrentMode)
        {
        case MatrixMode::Projection: m_Projection.Push(); break;
        case MatrixMode::Texture:    m_Texture.Push();    break;
        case MatrixMode::ModelView:
        default:                     m_ModelView.Push();  break;
        }
    }

    void Renderer::PopMatrix()
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        switch (m_CurrentMode)
        {
        case MatrixMode::Projection: m_Projection.Pop(); break;
        case MatrixMode::Texture:    m_Texture.Pop();    break;
        case MatrixMode::ModelView:
        default:                     m_ModelView.Pop();  break;
        }
    }

    void Renderer::LoadIdentity()
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        switch (m_CurrentMode)
        {
        case MatrixMode::Projection: m_Projection.LoadIdentity(); break;
        case MatrixMode::Texture:    m_Texture.LoadIdentity();    break;
        case MatrixMode::ModelView:
        default:                     m_ModelView.LoadIdentity();  break;
        }
    }

    void Renderer::LoadMatrix(const float* columnMajor)
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        if (columnMajor == nullptr)
        {
            return;
        }
        // glm::make_mat4 le 16 floats em ordem column-major, igual ao
        // glLoadMatrixf.
        const Mat4 m = glm::make_mat4(columnMajor);
        switch (m_CurrentMode)
        {
        case MatrixMode::Projection: m_Projection.Load(m); break;
        case MatrixMode::Texture:    m_Texture.Load(m);    break;
        case MatrixMode::ModelView:
        default:                     m_ModelView.Load(m);  break;
        }
    }

    void Renderer::MultMatrix(const float* columnMajor)
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        if (columnMajor == nullptr)
        {
            return;
        }
        const Mat4 m = glm::make_mat4(columnMajor);
        switch (m_CurrentMode)
        {
        case MatrixMode::Projection: m_Projection.Multiply(m); break;
        case MatrixMode::Texture:    m_Texture.Multiply(m);    break;
        case MatrixMode::ModelView:
        default:                     m_ModelView.Multiply(m);  break;
        }
    }

    void Renderer::Translatef(float x, float y, float z)
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        switch (m_CurrentMode)
        {
        case MatrixMode::Projection: m_Projection.Translate(x, y, z); break;
        case MatrixMode::Texture:    m_Texture.Translate(x, y, z);    break;
        case MatrixMode::ModelView:
        default:                     m_ModelView.Translate(x, y, z);  break;
        }
    }

    void Renderer::Rotatef(float angle, float x, float y, float z)
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        switch (m_CurrentMode)
        {
        case MatrixMode::Projection: m_Projection.Rotate(angle, x, y, z); break;
        case MatrixMode::Texture:    m_Texture.Rotate(angle, x, y, z);    break;
        case MatrixMode::ModelView:
        default:                     m_ModelView.Rotate(angle, x, y, z);  break;
        }
    }

    void Renderer::Scalef(float x, float y, float z)
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        switch (m_CurrentMode)
        {
        case MatrixMode::Projection: m_Projection.Scale(x, y, z); break;
        case MatrixMode::Texture:    m_Texture.Scale(x, y, z);    break;
        case MatrixMode::ModelView:
        default:                     m_ModelView.Scale(x, y, z);  break;
        }
    }

    void Renderer::SetProjectionPerspective(float fovY, float aspect, float zNear, float zFar)
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        // Equivalente a gluPerspective() no modo PROJECTION: a matriz do
        // topo e substituida (o glMultMatrix interno partiria da identidade).
        m_Projection.Load(MakePerspective(fovY, aspect, zNear, zFar));
    }

    void Renderer::SetProjectionOrtho2D(float left, float right, float bottom, float top)
    {
        if (m_BatchEnabled) { InvalidateBatch(); }
        m_Projection.Load(MakeOrtho2D(left, right, bottom, top));
    }

    void Renderer::GetModelView3x4(float out[3][4]) const
    {
        ToRowMajor3x4(m_ModelView.Top(), out);
    }

    // -----------------------------------------------------------------
    // Estado
    // -----------------------------------------------------------------

    void Renderer::ApplyStateToGL()
    {
        // NAO used no caminho de desenho.
        //
        // O motor legado ja aplica diretamente na GL o estado que existe no
        // core profile (GL_DEPTH_TEST, GL_BLEND + glBlendFunc, GL_CULL_FACE,
        // glDepthMask, glColorMask, glStencilOp, glScissor, glFrontFace,
        // glPolygonMode, glViewport). Reescrever esse estado aqui
        // sobrescreveria com valores do cache interno e quebraria o
        // resultado (por exemplo, o glBlendFunc de cada modo de blend).
        //
        // O que o core profile NAO suporta (textura por unidade, alpha test,
        // fog e environment de textura) e interceptedado pelo GfxGLAdapter e
        // entregue ao shader como uniforms.
    }

    void Renderer::ApplyUniforms()
    {
        m_Shader.Use();

        if (m_UseBindless)
        {
            // Cria o handle no primeiro DESENHO da textura (nao no BindTexture):
            // glGetTextureHandleARB retorna 0 se a textura ainda nao estiver
            // completa, e o jogo configura os parametros APOS o bind.
            const GLuint bound = m_State.IsTextureEnabled() ? m_CurrentBoundName : 0;
            if (bound != 0)
            {
                auto found = m_TextureHandles.find(bound);
                if (found == m_TextureHandles.end())
                {
                    const GLuint64 h = glGetTextureHandleARB(bound);
                    if (h != 0)
                    {
                        glMakeTextureHandleResidentARB(h);
                        m_TextureHandles[bound] = h;
                    }
                    found = m_TextureHandles.find(bound);
                }
                m_CurrentTextureHandle = (found != m_TextureHandles.end()) ? found->second : 0;
            }
            else
            {
                m_CurrentTextureHandle = 0;
            }

            if (m_UniformsDirty || m_CurrentTextureHandle != m_LastTextureHandle)
            {
                m_Shader.SetHandle("uTextureHandle", m_CurrentTextureHandle);
                m_LastTextureHandle = m_CurrentTextureHandle;
            }
        }

        // Matrizes
        const Mat4& projection = m_Projection.Top();
        const Mat4& modelView  = m_ModelView.Top();
        if (m_UniformsDirty || !MatricesEqual(projection, m_LastProjection))
        {
            m_Shader.SetMat4("uProjection", glm::value_ptr(projection));
            m_LastProjection = projection;
        }
        if (m_UniformsDirty || !MatricesEqual(modelView, m_LastModelView))
        {
            m_Shader.SetMat4("uModelView", glm::value_ptr(modelView));
            m_LastModelView = modelView;
        }

        const GLint useTexture = m_State.IsTextureEnabled() ? 1 : 0;
        if (m_UniformsDirty || useTexture != m_LastUseTexture)
        {
            m_Shader.SetInt("uUseTexture", useTexture);
            m_LastUseTexture = useTexture;
        }

        const GLint texEnvMode = static_cast<GLint>(m_State.GetTexEnvMode());
        if (m_UniformsDirty || texEnvMode != m_LastTexEnvMode)
        {
            m_Shader.SetInt("uTexEnvMode", texEnvMode);
            m_LastTexEnvMode = texEnvMode;
        }

        const GLint useAlphaTest = m_State.IsAlphaTestEnabled() ? 1 : 0;
        if (m_UniformsDirty || useAlphaTest != m_LastUseAlphaTest)
        {
            m_Shader.SetInt("uUseAlphaTest", useAlphaTest);
            m_LastUseAlphaTest = useAlphaTest;
        }

        const AlphaFunc alphaFuncRaw = m_State.GetAlphaFunc();
        // O enum interno de gfx::AlphaFunc (0..7) ja e o valor que o shader
        // espera em uAlphaFunc. Enviar os enum GL (GL_NEVER=0x0200...) faria
        // o shader cair no "else" (ALWAYS), desligando o alpha test.
        const GLint alphaFunc = static_cast<GLint>(alphaFuncRaw);
        if (m_UniformsDirty || alphaFunc != m_LastAlphaFunc)
        {
            m_Shader.SetInt("uAlphaFunc", alphaFunc);
            m_LastAlphaFunc = alphaFunc;
        }

        if (m_UniformsDirty || m_State.GetAlphaRef() != m_LastAlphaRef)
        {
            m_Shader.SetFloat("uAlphaRef", m_State.GetAlphaRef());
            m_LastAlphaRef = m_State.GetAlphaRef();
        }

        const GLint useFog = m_State.IsFogEnabled() ? 1 : 0;
        if (m_UniformsDirty || useFog != m_LastUseFog)
        {
            m_Shader.SetInt("uUseFog", useFog);
            m_LastUseFog = useFog;
        }

        if (m_UniformsDirty || m_State.GetFogStart() != m_LastFogStart)
        {
            m_Shader.SetFloat("uFogStart", m_State.GetFogStart());
            m_LastFogStart = m_State.GetFogStart();
        }

        if (m_UniformsDirty || m_State.GetFogEnd() != m_LastFogEnd)
        {
            m_Shader.SetFloat("uFogEnd", m_State.GetFogEnd());
            m_LastFogEnd = m_State.GetFogEnd();
        }

        GLfloat fogColor[4];
        m_State.GetFogColor(fogColor);
        if (m_UniformsDirty ||
            fogColor[0] != m_LastFogColor[0] ||
            fogColor[1] != m_LastFogColor[1] ||
            fogColor[2] != m_LastFogColor[2])
        {
            m_Shader.SetVec3("uFogColor", fogColor[0], fogColor[1], fogColor[2]);
            m_LastFogColor[0] = fogColor[0];
            m_LastFogColor[1] = fogColor[1];
            m_LastFogColor[2] = fogColor[2];
        }

        m_UniformsDirty = false;
    }

    // -----------------------------------------------------------------
    // Modo imediato
    // -----------------------------------------------------------------

    void Renderer::Begin(GLenum mode)
    {
        m_ImmediateActive = true;
        m_ImmediateMode = mode;
        m_Staging.clear();
    }

    void Renderer::End()
    {
        if (!m_ImmediateActive)
        {
            return;
        }
        m_ImmediateActive = false;
        EmitPrimitive(m_ImmediateMode, m_Staging.data(), m_Staging.size() / VERTEX_FLOATS);
        m_Staging.clear();
    }

    void Renderer::AppendVertex(float x, float y, float z)
    {
        m_Staging.push_back(x);
        m_Staging.push_back(y);
        m_Staging.push_back(z);

        m_Staging.push_back(m_CurrentTexCoord[0]);
        m_Staging.push_back(m_CurrentTexCoord[1]);

        m_Staging.push_back(m_CurrentColor[0]);
        m_Staging.push_back(m_CurrentColor[1]);
        m_Staging.push_back(m_CurrentColor[2]);
        m_Staging.push_back(m_CurrentColor[3]);
    }

    void Renderer::Vertex2f(float x, float y)
    {
        // ATENCAO: glVertex2f(x, y) NAO preserva o z corrente. A especificacao
        // define que as componentes ausentes recebem 0 (z) e 1 (w), ou seja,
        // glVertex2f(x, y) == glVertex3f(x, y, 0). Reutilizar o z do ultimo
        // vertice 3D jogava a UI 2D para fora do volume de recorte (o z vinha
        // de um vertice de modelo, ex.: -231) e os sprites desapareciam.
        m_CurrentVertex[0] = x;
        m_CurrentVertex[1] = y;
        m_CurrentVertex[2] = 0.0f;
        if (m_ImmediateActive)
        {
            AppendVertex(x, y, 0.0f);
        }
    }

    void Renderer::Vertex3f(float x, float y, float z)
    {
        m_CurrentVertex[0] = x;
        m_CurrentVertex[1] = y;
        m_CurrentVertex[2] = z;
        if (m_ImmediateActive)
        {
            AppendVertex(x, y, z);
        }
    }

    void Renderer::Vertex3fv(const float* v)
    {
        if (v == nullptr)
        {
            return;
        }
        m_CurrentVertex[0] = v[0];
        m_CurrentVertex[1] = v[1];
        m_CurrentVertex[2] = v[2];
        if (m_ImmediateActive)
        {
            AppendVertex(v[0], v[1], v[2]);
        }
    }

    void Renderer::TexCoord2f(float u, float v)
    {
        m_CurrentTexCoord[0] = u;
        m_CurrentTexCoord[1] = v;
    }

    void Renderer::TexCoord2fv(const float* v)
    {
        if (v == nullptr)
        {
            return;
        }
        m_CurrentTexCoord[0] = v[0];
        m_CurrentTexCoord[1] = v[1];
    }

    void Renderer::SetColor3f(float r, float g, float b)
    {
        m_CurrentColor[0] = r;
        m_CurrentColor[1] = g;
        m_CurrentColor[2] = b;
        m_CurrentColor[3] = 1.0f;
        m_State.SetCurrentColor(r, g, b, 1.0f);
    }

    void Renderer::SetColor4f(float r, float g, float b, float a)
    {
        m_CurrentColor[0] = r;
        m_CurrentColor[1] = g;
        m_CurrentColor[2] = b;
        m_CurrentColor[3] = a;
        m_State.SetCurrentColor(r, g, b, a);
    }

    void Renderer::SetColor3fv(const float* v)
    {
        if (v == nullptr)
        {
            return;
        }
        SetColor3f(v[0], v[1], v[2]);
    }

    void Renderer::SetColor4fv(const float* v)
    {
        if (v == nullptr)
        {
            return;
        }
        SetColor4f(v[0], v[1], v[2], v[3]);
    }

    void Renderer::SetColor3ub(GLubyte r, GLubyte g, GLubyte b)
    {
        SetColor4f(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    }

    void Renderer::SetColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
    {
        SetColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }

    void Renderer::Normal3f(float x, float y, float z)
    {
        m_CurrentNormal[0] = x;
        m_CurrentNormal[1] = y;
        m_CurrentNormal[2] = z;
    }

    void Renderer::Normal3fv(const float* v)
    {
        if (v == nullptr)
        {
            return;
        }
        Normal3f(v[0], v[1], v[2]);
    }

    // -----------------------------------------------------------------
    // Client-state arrays
    // -----------------------------------------------------------------

    void Renderer::EnableClientState(GLenum array)
    {
        switch (array)
        {
        case GL_VERTEX_ARRAY:        m_VertexArray.Enabled = true;  break;
        case GL_TEXTURE_COORD_ARRAY: m_TexCoordArray.Enabled = true; break;
        case GL_COLOR_ARRAY:         m_ColorArray.Enabled = true;   break;
        default: break;
        }
    }

    void Renderer::DisableClientState(GLenum array)
    {
        switch (array)
        {
        case GL_VERTEX_ARRAY:        m_VertexArray.Enabled = false;  break;
        case GL_TEXTURE_COORD_ARRAY: m_TexCoordArray.Enabled = false; break;
        case GL_COLOR_ARRAY:         m_ColorArray.Enabled = false;   break;
        default: break;
        }
    }

    void Renderer::VertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
    {
        m_VertexArray.Pointer = pointer;
        m_VertexArray.Size = size;
        m_VertexArray.Type = type;
        m_VertexArray.Stride = stride;
    }

    void Renderer::TexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
    {
        m_TexCoordArray.Pointer = pointer;
        m_TexCoordArray.Size = size;
        m_TexCoordArray.Type = type;
        m_TexCoordArray.Stride = stride;
    }

    void Renderer::ColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
    {
        m_ColorArray.Pointer = pointer;
        m_ColorArray.Size = size;
        m_ColorArray.Type = type;
        m_ColorArray.Stride = stride;
    }

    void Renderer::BuildFromClientArrays(GLenum mode, GLint first, GLsizei count)
    {
        m_Staging.clear();

        const ClientArray& va = m_VertexArray;
        const ClientArray& ta = m_TexCoordArray;
        const ClientArray& ca = m_ColorArray;

        if (!va.Enabled || va.Pointer == nullptr || va.Size < 2)
        {
            return;
        }

        // Os strides sao expressos em BYTES (como a OpenGL define). Quando o
        // jogo informa stride 0, os elementos sao consecutivos.
        const GLsizei vertexStride = (va.Stride != 0)
            ? va.Stride
            : static_cast<GLsizei>(sizeof(float) * va.Size);
        const GLsizei texStride = (ta.Stride != 0)
            ? ta.Stride
            : static_cast<GLsizei>(sizeof(float) * ta.Size);
        const GLsizei colorStride = (ca.Stride != 0)
            ? ca.Stride
            : static_cast<GLsizei>(sizeof(float) * ca.Size);

        m_Staging.reserve(static_cast<size_t>(count) * VERTEX_FLOATS);

        for (GLsizei i = 0; i < count; ++i)
        {
            const GLint index = first + i;

            // ATENCAO: o offset do array e calculado em BYTES. Usar a contagem
            // de floats por vertice aqui deslocava todos os vertices a partir
            // do segundo, corrompendo posicoes e coordenadas de textura.
            const GLfloat* v = reinterpret_cast<const GLfloat*>(
                static_cast<const char*>(va.Pointer) + static_cast<GLsizei>(index) * vertexStride);
            const float x = v[0];
            const float y = v[1];
            const float z = (va.Size >= 3) ? v[2] : 0.0f;

            float u = m_CurrentTexCoord[0];
            float vv = m_CurrentTexCoord[1];
            if (ta.Enabled && ta.Pointer != nullptr)
            {
                const GLfloat* t = reinterpret_cast<const GLfloat*>(
                    static_cast<const char*>(ta.Pointer) + static_cast<GLsizei>(index) * texStride);
                u = t[0];
                vv = (ta.Size >= 2) ? t[1] : 0.0f;
            }

            float r = m_CurrentColor[0];
            float g = m_CurrentColor[1];
            float b = m_CurrentColor[2];
            float a = m_CurrentColor[3];
            if (ca.Enabled && ca.Pointer != nullptr)
            {
                const GLfloat* c = reinterpret_cast<const GLfloat*>(
                    static_cast<const char*>(ca.Pointer) + static_cast<GLsizei>(index) * colorStride);
                r = c[0];
                g = c[1];
                b = (ca.Size >= 3) ? c[2] : 0.0f;
                a = (ca.Size >= 4) ? c[3] : 1.0f;
            }

            m_Staging.push_back(x);
            m_Staging.push_back(y);
            m_Staging.push_back(z);
            m_Staging.push_back(u);
            m_Staging.push_back(vv);
            m_Staging.push_back(r);
            m_Staging.push_back(g);
            m_Staging.push_back(b);
            m_Staging.push_back(a);

        }

        EmitPrimitive(mode, m_Staging.data(), m_Staging.size() / VERTEX_FLOATS);
        m_Staging.clear();
    }

    void Renderer::DrawArrays(GLenum mode, GLint first, GLsizei count)
    {


        if (count <= 0)
        {
            return;
        }
        BuildFromClientArrays(mode, first, count);
    }

    // -----------------------------------------------------------------
    // Desenho
    // -----------------------------------------------------------------

    void Renderer::AppendExpanded(std::vector<float>& out, GLenum mode,
                                  const float* vertices, size_t vertexCount)
    {
        const size_t needed = out.size() + vertexCount * 3 * VERTEX_FLOATS;
        if (out.capacity() < needed)
        {
            out.reserve(needed);
        }

        if (!NeedsTriangleExpansion(mode))
        {
            out.insert(out.end(), vertices, vertices + vertexCount * VERTEX_FLOATS);
            return;
        }
        if (mode == GL_QUADS)
        {
            for (size_t i = 0; i + 3 < vertexCount; i += 4)
            {
                const float* quad[4] = {
                    vertices + (i + 0) * VERTEX_FLOATS,
                    vertices + (i + 1) * VERTEX_FLOATS,
                    vertices + (i + 2) * VERTEX_FLOATS,
                    vertices + (i + 3) * VERTEX_FLOATS
                };
                const float* tri1[3] = { quad[0], quad[1], quad[2] };
                const float* tri2[3] = { quad[0], quad[2], quad[3] };
                for (int t = 0; t < 2; ++t)
                {
                    for (int k = 0; k < 3; ++k)
                    {
                        const float* v = (t == 0) ? tri1[k] : tri2[k];
                        out.insert(out.end(), v, v + VERTEX_FLOATS);
                    }
                }
            }
        }
        else if (mode == GL_TRIANGLE_FAN)
        {
            for (size_t i = 1; i + 1 < vertexCount; ++i)
            {
                const float* tri[3] = {
                    vertices + 0 * VERTEX_FLOATS,
                    vertices + i * VERTEX_FLOATS,
                    vertices + (i + 1) * VERTEX_FLOATS
                };
                for (int k = 0; k < 3; ++k)
                {
                    out.insert(out.end(), tri[k], tri[k] + VERTEX_FLOATS);
                }
            }
        }
        else // GL_POLYGON
        {
            for (size_t i = 0; i + 2 < vertexCount; i += 3)
            {
                const float* tri[3] = {
                    vertices + (i + 0) * VERTEX_FLOATS,
                    vertices + (i + 1) * VERTEX_FLOATS,
                    vertices + (i + 2) * VERTEX_FLOATS
                };
                for (int k = 0; k < 3; ++k)
                {
                    out.insert(out.end(), tri[k], tri[k] + VERTEX_FLOATS);
                }
            }
        }
    }

    void Renderer::FlushBatch()
    {
        if (m_Batch.empty())
        {
            return;
        }
        const size_t count = m_Batch.size() / VERTEX_FLOATS;
        const GLint first = static_cast<GLint>(m_Buffer.Upload(m_Batch.data(), count));
        if (first < 0) { m_Batch.clear(); return; }
        m_Buffer.Draw(GL_TRIANGLES, first, static_cast<GLsizei>(count));
        m_Batch.clear();
        ++m_GlDrawCount;
    }

    void Renderer::EmitPrimitive(GLenum mode, const float* vertices, size_t vertexCount)
    {

        static unsigned long s_DrawCount = 0;
        ++s_DrawCount;
        if (gfx::TraceIsEnabled() && (s_DrawCount % 2000) == 0)
        {
            // Razao entre primitivas recebidas e draws GL emitidos: quanto
            // maior, mais eficaz o batching (ex.: 5x = 5 primitivas/draw).
            Trace("[gfx] batch: prim=%lu glDraw=%llu ratio=%.2f enabled=%d\r\n",
                  s_DrawCount, m_GlDrawCount,
                  m_GlDrawCount ? (float)s_DrawCount / (float)m_GlDrawCount : 0.0f,
                  (int)m_BatchEnabled);
        }
        if (gfx::TraceIsEnabled() && (s_DrawCount <= 60 || (s_DrawCount % 500) == 0))
        {
            GLint boundTex = 0;
            gfx::raw::fnGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
            const GLboolean glTexOn = gfx::raw::fnIsEnabled(GL_TEXTURE_2D);
            Trace("[gfx] draw #%lu mode=0x%04X v=%u texOn=%d legadoCache=%d glTexOn=%d (enviado=%d) bound=%d texEnv=%d alphaTest=%d ref=%.2f",
                  s_DrawCount, (unsigned)mode, (unsigned)vertexCount,
                  (int)m_State.IsTextureEnabled(),
                  (int)TextureEnable,
                  (int)glTexOn,
                  (int)m_LastUseTexture,
                  (int)boundTex,
                  (int)m_State.GetTexEnvMode(),
                  (int)m_State.IsAlphaTestEnabled(),
                  (double)m_State.GetAlphaRef());
        }

        ApplyUniforms();

        // ---- Client arrays (Fase 2, alternativa ao mapped VBO) ----
        // Expande para triangulos em m_Expanded e desenha direto do CPU, sem
        // VBO/upload. O driver copia internamente no glDrawArrays (compat profile).
        if (m_ClientArray)
        {
            std::vector<float>& out = m_Expanded;
            out.clear();
            AppendExpanded(out, mode, vertices, vertexCount);
            if (!out.empty())
            {
                m_Buffer.DrawClient(GL_TRIANGLES, out.data(),
                                    static_cast<GLsizei>(out.size() / VERTEX_FLOATS));
                ++m_GlDrawCount;
            }
            return;
        }

        // ---- Batching (Fase 2) ----
        // Acumula primitivas com o MESMO estado num unico draw. O estado
        // e representado por m_StateGen: hooks no adapter incrementam o
        // contador a cada mudanca (textura, blend, depth, matriz, ...).
        // Quando a geracao muda, o lote anterior e desenhado antes de
        // acumular os novos vertices.
        if (m_BatchEnabled)
        {
            // Fecha o lote anterior se o estado mudou (defensivo: os hooks
            // do adapter ja fazem isso no momento da mudanca).
            if (!m_Batch.empty() && m_BatchGen != m_StateGen)
            {
                FlushBatch();
            }
            m_BatchGen = m_StateGen;

            // Expande direto em m_Batch - sem copia intermediaria.
            const size_t before = m_Batch.size();
            AppendExpanded(m_Batch, mode, vertices, vertexCount);
            if (m_Batch.size() == before)
            {
                return;
            }

            // Limita o tamanho do lote: evita um unico draw gigante.
            if (m_Batch.size() >= 65536 * VERTEX_FLOATS)
            {
                FlushBatch();
            }
            return;
        }

        // ---- Caminho sem batching (Fase 1, comportamento original) ----
        if (!NeedsTriangleExpansion(mode))
        {
            const GLenum coreMode = ToCorePrimitive(mode);
            if (coreMode == 0)
            {
                return;
            }
            const GLint first = static_cast<GLint>(m_Buffer.Upload(vertices, vertexCount));
            if (first < 0) { return; }
            m_Buffer.Draw(coreMode, first, static_cast<GLsizei>(vertexCount));
            ++m_GlDrawCount;
            return;
        }

        // ---- Expansao para triangulos (core profile) ----
        // O buffer de expansao e um membro reaproveitado: alocar um
        // std::vector por primitiva significava milhares de alocacoes por
        // frame e era um dos custos medidos no modo moderno.
        std::vector<float>& expanded = m_Expanded;
        expanded.clear();
        if (expanded.capacity() < vertexCount * 3 * VERTEX_FLOATS)
        {
            expanded.reserve(vertexCount * 3 * VERTEX_FLOATS);
        }

        if (mode == GL_QUADS)
        {
            // (v0,v1,v2) + (v0,v2,v3) - preserva o winding original.
            for (size_t i = 0; i + 3 < vertexCount; i += 4)
            {
                const float* quad[4] = {
                    vertices + (i + 0) * VERTEX_FLOATS,
                    vertices + (i + 1) * VERTEX_FLOATS,
                    vertices + (i + 2) * VERTEX_FLOATS,
                    vertices + (i + 3) * VERTEX_FLOATS
                };

                const float* tri1[3] = { quad[0], quad[1], quad[2] };
                const float* tri2[3] = { quad[0], quad[2], quad[3] };

                for (int t = 0; t < 2; ++t)
                {
                    for (int k = 0; k < 3; ++k)
                    {
                        const float* vertex = (t == 0) ? tri1[k] : tri2[k];
                        expanded.insert(expanded.end(), vertex, vertex + VERTEX_FLOATS);
                    }
                }
            }
        }
        else if (mode == GL_TRIANGLE_FAN)
        {
            // (v0, vi, vi+1) para i = 1..N-2.
            for (size_t i = 1; i + 1 < vertexCount; ++i)
            {
                const float* tri[3] = {
                    vertices + 0 * VERTEX_FLOATS,
                    vertices + i * VERTEX_FLOATS,
                    vertices + (i + 1) * VERTEX_FLOATS
                };
                for (int k = 0; k < 3; ++k)
                {
                    const float* vertex = tri[k];
                    expanded.insert(expanded.end(), vertex, vertex + VERTEX_FLOATS);
                }
            }
        }
        else // GL_POLYGON - nao usado pelo motor, tratado como triangulos.
        {
            for (size_t i = 0; i + 2 < vertexCount; i += 3)
            {
                const float* tri[3] = {
                    vertices + (i + 0) * VERTEX_FLOATS,
                    vertices + (i + 1) * VERTEX_FLOATS,
                    vertices + (i + 2) * VERTEX_FLOATS
                };
                for (int k = 0; k < 3; ++k)
                {
                    const float* vertex = tri[k];
                    expanded.insert(expanded.end(), vertex, vertex + VERTEX_FLOATS);
                }
            }
        }

        if (expanded.empty())
        {
            return;
        }

        const size_t expandedCount = expanded.size() / VERTEX_FLOATS;
        const GLint first = static_cast<GLint>(m_Buffer.Upload(expanded.data(), expandedCount));
        if (first < 0) { return; }
        m_Buffer.Draw(GL_TRIANGLES, first, static_cast<GLsizei>(expandedCount));
        ++m_GlDrawCount;
    }
}

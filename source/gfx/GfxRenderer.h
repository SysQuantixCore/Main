//======================================================================
// gfx/GfxRenderer.h
//
// Renderer grafico do backend moderno.
//
// Substitui o pipeline fixo por VBOs/VAOs + shaders GLSL 3.3 core.
//
// Este objeto e a implementacao do "backend moderno". O backend legado
// (fixed-function) continua intacto em ZzzOpenglUtil.cpp e e selecionado
// em tempo de execucao.
//
// O renderer expoe exatamente o conjunto de primitivas que o motor legado
// usava, para que o jogo nao precise conhecer detalhes de OpenGL:
//
//   - Pilhas de matrizes (MODELVIEW / PROJECTION / TEXTURE) -> uniforms.
//   - Modo imediato (Begin/End + atributos) -> VBO dinamico.
//   - Client-state arrays (VertexPointer/ColorPointer/...) -> VBO dinamico.
//   - Estado (blend, depth, cull, alpha test, fog, texenv) -> uniforms.
//
// Conversoes de primitiva (obrigatorias em core profile, onde GL_QUADS e
// GL_TRIANGLE_FAN nao existem):
//   GL_QUADS         -> 2 triangulos por quad
//   GL_TRIANGLE_FAN  -> N-2 triangulos
// A conversao preserva a ordem dos vertices (winding) e portanto o
// resultado do culling e do depth test.
//======================================================================
#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "gfx/GfxBuffer.h"
#include "gfx/GfxMath.h"
#include "gfx/GfxShader.h"
#include "gfx/GfxState.h"
#include "gfx/GfxTypes.h"

namespace gfx
{
    class Renderer
    {
    public:
        static Renderer& Instance();

        // ---------------- Inicializacao ----------------
        bool InitializeModern(std::string& errorLog);
        void Shutdown();

        bool IsModern() const { return m_Modern; }
        void SetLegacy() { m_Modern = false; }

        // ---------------- Pilhas de matrizes ----------------
        void SetMatrixMode(MatrixMode mode);
        MatrixMode GetMatrixMode() const { return m_CurrentMode; }

        void PushMatrix();
        void PopMatrix();
        void LoadIdentity();
        void LoadMatrix(const float* columnMajor);
        void MultMatrix(const float* columnMajor);
        void Translatef(float x, float y, float z);
        void Rotatef(float angle, float x, float y, float z);
        void Scalef(float x, float y, float z);

        void SetProjectionPerspective(float fovY, float aspect, float zNear, float zFar);
        void SetProjectionOrtho2D(float left, float right, float bottom, float top);

        const Mat4& GetModelViewMatrix() const { return m_ModelView.Top(); }
        const Mat4& GetProjectionMatrix() const { return m_Projection.Top(); }

        // Converte a matriz modelview para o formato float[3][4] row-major
        // usado pelo jogo (identico a GetOpenGLMatrix do caminho legado).
        void GetModelView3x4(float out[3][4]) const;

        // ---------------- Estado ----------------
        State& GetState() { return m_State; }
        const State& GetState() const { return m_State; }

        Shader& GetShader() { return m_Shader; }
        GfxBuffer& GetBuffer() { return m_Buffer; }

        // ---------------- Bindless (Fase 2) ----------------
        bool IsBindless() const { return m_UseBindless; }
        // Registra a textura 'name' como atual e devolve seu handle 64-bit.
        // Chamado pelo adapter em gfxGLBindTexture no caminho moderno.
        GLuint64 SetTexture(GLuint name);
        void SetBoundTextureName(GLuint name) { m_CurrentBoundName = name; }

        // ---------------- Modo imediato ----------------
        void Begin(GLenum mode);
        void End();

        void Vertex2f(float x, float y);
        void Vertex3f(float x, float y, float z);
        void Vertex3fv(const float* v);
        void TexCoord2f(float u, float v);
        void TexCoord2fv(const float* v);
        void SetColor3f(float r, float g, float b);
        void SetColor4f(float r, float g, float b, float a);
        void SetColor3fv(const float* v);
        void SetColor4fv(const float* v);
        void SetColor3ub(GLubyte r, GLubyte g, GLubyte b);
        void SetColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a);
        void Normal3f(float x, float y, float z);
        void Normal3fv(const float* v);

        // ---------------- Client-state arrays ----------------
        void EnableClientState(GLenum array);
        void DisableClientState(GLenum array);

        void VertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
        void TexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);
        void ColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer);

        void DrawArrays(GLenum mode, GLint first, GLsizei count);

        // ---------------- Consultas ----------------
        void GetCurrentColor(float out[4]) const { m_State.GetCurrentColor(out); }

        // Programa GLSL atualmente em uso (0 = nenhum / fixed-function).
        static GLint currentProgram();


        // Le o depth buffer do framebuffer atual.
        //
        // Em core profile glReadPixels(GL_DEPTH_COMPONENT) no framebuffer
        // padrao e ilegal. A solucao e copiar a regiao do depth buffer para
        // uma textura de profundidade via glBlitFramebuffer e ler a textura.
        //
        // Retorna false se nao foi possivel (neste caso o chamador assume
        // "sem oclusao").
        bool ReadDepth(GLint x, GLint y, GLsizei width, GLsizei height, GLfloat* out);

        // ---------------- Batching (Fase 2) ----------------
        // Chamado pelo adapter quando o estado muda: desenha o lote acumulado
        // antes que a mudanca seja aplicada (textura/blend/matriz/etc.).
        void FlushBatch();
        void InvalidateBatch()
        {
            FlushBatch();
            ++m_StateGen;
        }
        bool IsBatching() const { return m_BatchEnabled; }

    private:
        Renderer() = default;

        bool EnsureDepthMirror();

        // Modo imediato: concatena vertices num buffer de staging.
        void AppendVertex(float x, float y, float z);

        // Expande QUADS/FAN/POLYGON em triangulos dentro de 'out'.
        void AppendExpanded(std::vector<float>& out, GLenum mode,
                            const float* vertices, size_t vertexCount);

        // Client arrays: le os arrays do cliente e monta o staging.
        void BuildFromClientArrays(GLenum mode, GLint first, GLsizei count);

        // Converte a primitiva original em triangulos (core profile).
        void EmitPrimitive(GLenum mode, const float* vertices, size_t vertexCount);

        void FlushStaging(GLenum mode);
        void ApplyUniforms();
        void ApplyStateToGL();

        // ---------------- Dados ----------------
        bool  m_Modern = false;

        State       m_State;
        MatrixStack m_ModelView;
        MatrixStack m_Projection;
        MatrixStack m_Texture;
        MatrixMode  m_CurrentMode = MatrixMode::ModelView;

        Shader   m_Shader;
        GfxBuffer m_Buffer;

        // Bindless textures (Fase 2): nome GL -> handle 64-bit residente.
        bool m_UseBindless = false;
        GLuint m_CurrentBoundName = 0;          // ultimo nome ligado pelo jogo
        GLuint64 m_CurrentTextureHandle = 0;
        std::unordered_map<GLuint, GLuint64> m_TextureHandles;

        // Modo imediato
        bool   m_ImmediateActive = false;
        GLenum m_ImmediateMode = GL_TRIANGLES;
        std::vector<float> m_Staging;
        // Buffer reaproveitado da expansao QUADS/FAN -> triangulos.
        std::vector<float> m_Expanded;

        // Batching: vertices acumulados do lote corrente.
        std::vector<float> m_Batch;
        bool   m_BatchEnabled = false;
        unsigned long long m_StateGen = 0;   // incrementado a cada mudanca de estado
        unsigned long long m_BatchGen = 0;   // geracao do lote acumulado
        unsigned long long m_GlDrawCount = 0; // draws GL emitidos (para medir batching)

        // Atributos correntes (persistentes entre Begin/End, como no GL 1.1).
        float m_CurrentVertex[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float m_CurrentTexCoord[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        float m_CurrentColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float m_CurrentNormal[3] = { 0.0f, 0.0f, 1.0f };

        // Client-state arrays
        struct ClientArray
        {
            const void* Pointer = nullptr;
            GLint      Size = 0;
            GLenum     Type = GL_FLOAT;
            GLsizei    Stride = 0;
            bool       Enabled = false;
        };
        ClientArray m_VertexArray;
        ClientArray m_TexCoordArray;
        ClientArray m_ColorArray;

        // Cache das uniforms para evitar glUniform* redundante.
        bool  m_UniformsDirty = true;
        GLint m_LastUseTexture = -1;
        GLuint64 m_LastTextureHandle = 0xFFFFFFFFFFFFFFFFULL;
        GLint m_LastTexEnvMode = -1;
        GLint m_LastUseAlphaTest = -1;
        GLint m_LastAlphaFunc = -1;
        GLfloat m_LastAlphaRef = -1.0f;
        GLint m_LastUseFog = -1;
        GLfloat m_LastFogStart = -1.0f;
        GLfloat m_LastFogEnd = -1.0f;
        GLfloat m_LastFogColor[3] = { -1.0f, -1.0f, -1.0f };
        Mat4 m_LastProjection;
        Mat4 m_LastModelView;

        // Espelho do depth buffer (1x1) usado pela leitura de profundidade.
        GLuint m_DepthFbo = 0;
        GLuint m_DepthTexture = 0;
        GLenum m_DepthTextureFormat = 0;
        bool   m_DepthMirrorWarned = false;
    };
}

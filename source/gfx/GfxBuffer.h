//======================================================================
// gfx/GfxBuffer.h
//
// VAO + VBO do backend moderno.
//
// Layout de vertice (intercalado, 9 floats = 36 bytes):
//   offset 0 : vec3 posicao
//   offset 3 : vec2 texcoord
//   offset 5 : vec4 cor
//
// O VBO e dinamico (GL_DYNAMIC_DRAW): o motor legacy atualiza os vertices
// a cada quadro (skinning na CPU), entao nao ha ganho em alocar buffer
// estatico por malha nesta fase da migracao.
//======================================================================
#pragma once

#include <cstddef>

#include "gfx/GfxTypes.h"

namespace gfx
{
    class GfxBuffer
    {
    public:
        GfxBuffer() = default;
        ~GfxBuffer();

        GfxBuffer(const GfxBuffer&) = delete;
        GfxBuffer& operator=(const GfxBuffer&) = delete;

        bool Create(size_t initialVertexCapacity);
        void Release();

        bool IsValid() const { return m_Vao != 0 && m_Vbo != 0; }

        // Envia vertexCount vertices (9 floats cada) para a GPU.
        void Upload(const float* vertices, size_t vertexCount);

        // Desenha 'count' vertices a partir de 'first'.
        void Draw(GLenum mode, GLint first, GLsizei count);

        void Bind() const;

    private:
        GLuint m_Vao = 0;
        GLuint m_Vbo = 0;
        size_t m_CapacityVertices = 0;
    };
}

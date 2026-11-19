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

        // Ativa/desativa o caminho persistently mapped (ring buffer).
        // Chamado antes de Create(). Desligado = glBufferSubData classico.
        void SetMapped(bool enabled) { m_Mapped = enabled; }

        // Envia vertexCount vertices (9 floats cada) para a GPU.
        // No caminho mapeado, devolve o indice do primeiro vertice escrito
        // no ring buffer (para uso como 'first' em Draw).
        size_t Upload(const float* vertices, size_t vertexCount);

        // Desenha 'count' vertices a partir de 'first'.
        void Draw(GLenum mode, GLint first, GLsizei count);

        void Bind() const;
        // Desvincula o VAO antes de codigo que usa VAO proprio (ImGui/ScaleForm).
        void Unbind();

        // Inicio de frame: orphana o buffer para reescrever do inicio sem
        // sobrescrever dados que a GPU ainda esteja usando.
        void BeginFrame();

    private:
        GLuint m_Vao = 0;
        GLuint m_Vbo = 0;
        size_t m_CapacityVertices = 0;

        // Caminho persistently mapped (Fase 2)
        bool   m_Mapped = false;
        float* m_MappedPtr = nullptr;   // região mapeada do VBO
        size_t m_MappedWrite = 0;       // cursor de escrita no ring buffer
        size_t m_RegionStart = 0;       // inicio da região do frame atual
        size_t m_RegionSize = 0;        // tamanho de cada região (capacity / 4)
        int    m_RegionIndex = 0;       // região atual (0..3)
        // Fences: sinalizam quando a GPU terminou de ler cada região. Antes de
        // reutilizar uma região, esperamos seu fence (1 wait/frame, barato).
        GLsync m_RegionFences[4] = { nullptr, nullptr, nullptr, nullptr };
        bool   m_VaoBound = false;      // VAO ja vinculado (evita bind por draw)
    };
}

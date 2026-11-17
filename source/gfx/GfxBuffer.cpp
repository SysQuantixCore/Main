//======================================================================
// gfx/GfxBuffer.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxBuffer.h"
#include "gfx/GfxRawGL.h"

namespace gfx
{
    namespace
    {
        constexpr size_t kVertexStride = sizeof(float) * VERTEX_FLOATS;
    }

    GfxBuffer::~GfxBuffer()
    {
        Release();
    }

    bool GfxBuffer::Create(size_t initialVertexCapacity)
    {
        Release();

        if (initialVertexCapacity == 0)
        {
            initialVertexCapacity = 1024;
        }

        glGenVertexArrays(1, &m_Vao);
        glGenBuffers(1, &m_Vbo);
        if (m_Vao == 0 || m_Vbo == 0)
        {
            Release();
            return false;
        }

        glBindVertexArray(m_Vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(initialVertexCapacity * kVertexStride),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        const GLsizei stride = static_cast<GLsizei>(kVertexStride);

        glEnableVertexAttribArray(ATTRIB_POSITION);
        glVertexAttribPointer(ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<const void*>(sizeof(float) * VERTEX_POS_OFFSET));

        glEnableVertexAttribArray(ATTRIB_TEXCOORD);
        glVertexAttribPointer(ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<const void*>(sizeof(float) * VERTEX_TEXCOORD_OFFSET));

        glEnableVertexAttribArray(ATTRIB_COLOR);
        glVertexAttribPointer(ATTRIB_COLOR, 4, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<const void*>(sizeof(float) * VERTEX_COLOR_OFFSET));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_CapacityVertices = initialVertexCapacity;
        return true;
    }

    void GfxBuffer::Release()
    {
        if (m_Vbo != 0)
        {
            glDeleteBuffers(1, &m_Vbo);
            m_Vbo = 0;
        }
        if (m_Vao != 0)
        {
            glDeleteVertexArrays(1, &m_Vao);
            m_Vao = 0;
        }
        m_CapacityVertices = 0;
    }

    void GfxBuffer::Upload(const float* vertices, size_t vertexCount)
    {
        if (m_Vbo == 0 || vertices == nullptr || vertexCount == 0)
        {
            return;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);

        const size_t requiredBytes = vertexCount * kVertexStride;
        if (vertexCount > m_CapacityVertices)
        {
            // Cresce em potencias de dois para evitar realocacoes frequentes.
            size_t newCapacity = (m_CapacityVertices > 0) ? m_CapacityVertices : 1024;
            while (newCapacity < vertexCount)
            {
                newCapacity *= 2;
            }
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(newCapacity * kVertexStride),
                         nullptr,
                         GL_DYNAMIC_DRAW);
            m_CapacityVertices = newCapacity;
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(requiredBytes), vertices);
        // O binding de GL_ARRAY_BUFFER e deixado como esta: o VAO guarda os
        // ponteiros dos atributos e desfazer o binding a cada draw custa uma
        // chamada de driver por primitiva.
    }

    void GfxBuffer::Draw(GLenum mode, GLint first, GLsizei count)
    {
        if (m_Vao == 0 || count <= 0)
        {
            return;
        }

        glBindVertexArray(m_Vao);

        // IMPORTANTE: usar a funcao CRUA (gfx::raw::fnDrawArrays) e nao
        // glDrawArrays. A macro de redirecionamento do GfxGLAdapter.h
        // transforma "glDrawArrays" em "gfxGLDrawArrays", que voltaria para
        // gfx::Renderer::DrawArrays -> BuildFromClientArrays -> EmitPrimitive
        // -> GfxBuffer::Draw -> ... RECURSAO INFINITA (stack overflow).
        gfx::raw::fnDrawArrays(mode, first, count);

        glBindVertexArray(0);
    }

    void GfxBuffer::Bind() const
    {
        if (m_Vao != 0)
        {
            glBindVertexArray(m_Vao);
        }
    }
}

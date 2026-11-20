//======================================================================
// gfx/GfxBuffer.cpp
//======================================================================
#include "stdafx.h"
#include "gfx/GfxBuffer.h"
#include "gfx/GfxRawGL.h"
#include "gfx/GfxTrace.h"

#include <cstring>

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

        if (m_Mapped)
        {
            // Persistently mapped exige ARB_buffer_storage (GL 4.4+).
            // Se o driver so tem 3.3, a extensao pode estar ausente: nesse
            // caso caimos para o caminho classico glBufferData/SubData.
            const bool hasStorage = (GLEW_ARB_buffer_storage != 0) &&
                                    (glBufferStorage != nullptr);
            if (hasStorage)
            {
                // Tentativa: COM COHERENT + flush explicito. O driver Intel
                // rejeita glFlushMappedBufferRange (0x0502) em mapas PERSISTENT
                // sem COHERENT, entao a escrita nao fica visivel. Com COHERENT
                // o flush e redundante mas inofensivo; o bind do VBO (correcao
                // anterior) pode ter sido a causa real da corrupcao.
                const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT |
                                         GL_MAP_COHERENT_BIT;
                glBufferStorage(GL_ARRAY_BUFFER,
                                static_cast<GLsizeiptr>(initialVertexCapacity * kVertexStride),
                                nullptr, flags);
                m_MappedPtr = static_cast<float*>(
                    glMapBufferRange(GL_ARRAY_BUFFER, 0,
                                     static_cast<GLsizeiptr>(initialVertexCapacity * kVertexStride),
                                     flags));
                if (m_MappedPtr == nullptr)
                {
                    m_Mapped = false;
                }
            }
            else
            {
                m_Mapped = false;   // sem extensao -> caminho classico
            }

            if (!m_Mapped)
            {
                glBufferData(GL_ARRAY_BUFFER,
                             static_cast<GLsizeiptr>(initialVertexCapacity * kVertexStride),
                             nullptr, GL_DYNAMIC_DRAW);
            }
        }
        else
        {
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(initialVertexCapacity * kVertexStride),
                         nullptr, GL_DYNAMIC_DRAW);
        }

        m_MappedWrite = 0;

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
        // 4 regioes independentes (uma por frame): a GPU fica no maximo 3
        // frames atras, entao nunca reescrevemos uma regiao em uso.
        m_RegionSize = m_Mapped ? (m_CapacityVertices / 4) : 0;
        m_RegionStart = 0;
        m_RegionIndex = 0;
        return true;
    }

    void GfxBuffer::Release()
    {
        if (m_MappedPtr != nullptr)
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            m_MappedPtr = nullptr;
        }
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
        for (int i = 0; i < 4; ++i)
        {
            if (m_RegionFences[i] != nullptr)
            {
                glDeleteSync(m_RegionFences[i]);
                m_RegionFences[i] = nullptr;
            }
        }
        m_MappedWrite = 0;
        m_CapacityVertices = 0;
    }

    size_t GfxBuffer::Upload(const float* vertices, size_t vertexCount)
    {
        if (m_Vbo == 0 || vertices == nullptr || vertexCount == 0)
        {
            return static_cast<size_t>(-1);
        }

        const size_t requiredBytes = vertexCount * kVertexStride;

        if (m_Mapped && m_MappedPtr != nullptr)
        {
            // Escreve dentro da regiao do frame atual. Se nao cabe no fim da
            // regiao, volta ao inicio DELA (nao do buffer todo). Regiao grande
            // o suficiente para um frame inteiro => wrap dentro do frame e raro.
            const size_t regionEnd = m_RegionStart + m_RegionSize;
            if (m_MappedWrite + vertexCount > regionEnd)
            {
                m_MappedWrite = m_RegionStart;
            }
            // Capacidade da regiao insuficiente (primitiva gigante rara): ignora.
            if (vertexCount > m_RegionSize)
            {
                return static_cast<size_t>(-1);
            }
            const size_t offsetFloats = m_MappedWrite * VERTEX_FLOATS;
            float* dst = m_MappedPtr + offsetFloats;
            memcpy(dst, vertices, requiredBytes);
            // glFlushMappedBufferRange exige que o buffer mapeado esteja
            // vinculado a GL_ARRAY_BUFFER. O Draw so vincula o VAO, entao o
            // binding pode estar em 0 aqui -> sem isso, flush gera
            // GL_INVALID_OPERATION e a escrita NAO fica visivel ao GPU.
            glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
            glFlushMappedBufferRange(GL_ARRAY_BUFFER,
                                     static_cast<GLintptr>(offsetFloats * sizeof(float)),
                                     static_cast<GLsizeiptr>(requiredBytes));
            const size_t first = m_MappedWrite;
            m_MappedWrite += vertexCount;
            return first;
        }

        // Caminho classico (glBufferSubData).
        glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);

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
        return 0;  // caminho classico: vertices sempre em offset 0
    }

    void GfxBuffer::Draw(GLenum mode, GLint first, GLsizei count)
    {
        if (m_Vao == 0 || count <= 0)
        {
            return;
        }

        // Mantem o VAO vinculado entre draws. Fazer bind/unbind a cada
        // primitiva custava 2 chamadas de driver por draw (centenas/frame).
        if (!m_VaoBound)
        {
            glBindVertexArray(m_Vao);
            m_VaoBound = true;
        }

        // IMPORTANTE: usar a funcao CRUA (gfx::raw::fnDrawArrays) e nao
        // glDrawArrays. A macro de redirecionamento do GfxGLAdapter.h
        // transforma "glDrawArrays" em "gfxGLDrawArrays", que voltaria para
        // gfx::Renderer::DrawArrays -> BuildFromClientArrays -> EmitPrimitive
        // -> GfxBuffer::Draw -> ... RECURSAO INFINITA (stack overflow).
        gfx::raw::fnDrawArrays(mode, first, count);
    }

    void GfxBuffer::BeginFrame()
    {
        if (!m_Mapped || m_MappedPtr == nullptr || m_RegionSize == 0)
        {
            return;
        }

        // 1) Sinaliza que a GPU terminou de LER a regiao do frame que acabou
        //    (todos os glDrawArrays desse frame já foram submetidos).
        if (m_RegionFences[m_RegionIndex] == nullptr)
        {
            m_RegionFences[m_RegionIndex] =
                glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        }

        // 2) Avanca para a proxima regiao e ESPERA seu fence. Isso garante que
        //    a GPU nao esteja mais lendo essa regiao antes de reescrevê-la.
        //    Custo: 1 wait por frame (vs ~600 glBufferSubData eliminados).
        m_RegionIndex = (m_RegionIndex + 1) % 4;
        if (m_RegionFences[m_RegionIndex] != nullptr)
        {
            // Espera BLOQUEANTE: timeout 0 so verifica e nao garante que a GPU
            // terminou de ler a regiao -> corrompia a imagem. Com timeout
            // grande, bloqueamos ate o fence sinalizar (1 wait/frame).
            glClientWaitSync(m_RegionFences[m_RegionIndex],
                             GL_SYNC_FLUSH_COMMANDS_BIT, 1000000000ULL); // 1s
            glDeleteSync(m_RegionFences[m_RegionIndex]);
            m_RegionFences[m_RegionIndex] = nullptr;
        }

        m_RegionStart = m_RegionIndex * m_RegionSize;
        m_MappedWrite = m_RegionStart;
    }

    void GfxBuffer::Unbind()
    {
        if (m_VaoBound)
        {
            glBindVertexArray(0);
            m_VaoBound = false;
        }
    }

    void GfxBuffer::Bind() const
    {
        if (m_Vao != 0)
        {
            glBindVertexArray(m_Vao);
        }
    }
}

//======================================================================
// gfx/GfxTypes.h
//
// Tipos base do modulo de abstracao grafica (gfx/).
//
// Objetivo: isolar o cliente MU do OpenGL. Nada aqui deve incluir
// headers do jogo (GlobalBitmap, ZzzOpenglUtil, ...); o modulo gfx e
// autonome e apenas expoe tipos proprios + uma camada fina sobre GL/GLM.
//
// Convencoes importantes (herdadas do motor original):
//   - Matrizes 4x4 em ordem COLUMN-MAJOR (igual ao OpenGL e ao GLM).
//     O array bruto enviado a GPU e o mesmo que glLoadMatrixf/glUniformMatrix4fv
//     esperam.
//   - O pipeline de vertices e sempre  clip = P * M * v
//     (projection * modelview * vertex), identico ao fixed-function.
//======================================================================
#pragma once

// Headers base do OpenGL usados por todo o modulo gfx.
// windows.h precisa vir antes de glew.h (exigencia do GLEW no Windows).
#include <windows.h>
#include <gl/glew.h>

#include <cstddef>

namespace gfx
{
    // Numero de floats por vertice no layout interno do renderer moderno.
    //   0..2  posicao  (x, y, z)
    //   3..4  texcoord (u, v)
    //   5..8  cor      (r, g, b, a)
    enum
    {
        VERTEX_FLOATS = 9,
        VERTEX_POS_OFFSET   = 0,
        VERTEX_TEXCOORD_OFFSET = 3,
        VERTEX_COLOR_OFFSET  = 5,
    };

    // Vertex shader attribute locations (espelham o layout acima).
    enum
    {
        ATTRIB_POSITION = 0,
        ATTRIB_TEXCOORD = 1,
        ATTRIB_COLOR    = 2,
    };

    // Modo de environment de textura. O fixed-function original usava
    // GL_MODULATE (padrao em todas as texturas) e GL_ADD em um unico lugar
    // (ZzzEffectParticle.cpp - fumaca avancada).
    enum TexEnvMode
    {
        TEXENV_MODULATE = 0,
        TEXENV_ADD      = 1,
        TEXENV_REPLACE  = 2,
    };

    // Funcao de alpha test replicando glAlphaFunc.
    // O motor original usa exclusivamente GL_GREATER.
    enum AlphaFunc
    {
        ALPHAFUNC_NEVER    = 0,
        ALPHAFUNC_LESS     = 1,
        ALPHAFUNC_EQUAL    = 2,
        ALPHAFUNC_LEQUAL   = 3,
        ALPHAFUNC_GREATER  = 4,
        ALPHAFUNC_NOTEQUAL = 5,
        ALPHAFUNC_GEQUAL   = 6,
        ALPHAFUNC_ALWAYS   = 7,
    };

    // Converte a contagem de componentes de uma textura (3 = RGB, 4 = RGBA)
    // para o internal format "sized" exigido pelo core profile.
    //
    // CONTEXTO IMPORTANTE
    // ------------------
    // O motor legado usava o internal format nao sized (GL_RGB = 3,
    // GL_RGBA = 4). Isso funciona em contexto de COMPATIBILIDADE, mas em
    // core profile (3.2+) glTexImage2D com internal format nao sized gera
    // GL_INVALID_ENUM e a textura NAO e criada (fica incompleta).
    //
    // Core profile exige:
    //   GL_RGB  -> GL_RGB8
    //   GL_RGBA -> GL_RGBA8
    inline GLenum SizedInternalFormat(GLuint components)
    {
        return (components == 4) ? GL_RGBA8 : GL_RGB8;
    }
}
